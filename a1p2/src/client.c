
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <netdb.h>
#include "protocol.h"

void* network_thread_actor(void* arg){ // thread_actor, thread that performs user i/o and send operations
    // get pointer to thread common data, define thread variables and static buffers
    clientdata* client = (clientdata*)arg;
    char input[INPUT_MAX];
    char hout;
    net_buffer buffer_send;

    // send argv name to server
    buffer_seek(&buffer_send,0);
    hout = HEADER_INFO;
    buffer_write(&buffer_send,&hout,sizeof(char));
    buffer_write_string(&buffer_send,client->name);
    send(client->socket,buffer_send.buffer,buffer_tell(&buffer_send),0);

    // i/o loop
    while (client->terminate == 0) {
        // get input
        buffer_seek(&buffer_send,0);
        memset(input, '\0', sizeof(input));
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        // process input
        if(client->state == GAME_STATE_WAIT){
            if(strcmp(input, "quit") == 0){
                // user quits
                hout = HEADER_END;
                buffer_write(&buffer_send,&hout,sizeof(char));
                client->terminate = 1;
            }else{
                // user sends text message. i couldve easily made this a protocol error but ip messaging is fun
                hout = HEADER_TEXT;
                buffer_write(&buffer_send,&hout,sizeof(char));
                buffer_write_string(&buffer_send,input/*+sizeof(char)*/);
            }
        }else if(client->state == GAME_STATE_GO){
            // the client doesn't need to sanitize its own input because the server knows
            hout = HEADER_MOVE;
            buffer_write(&buffer_send,&hout,sizeof(char));
            buffer_write_string(&buffer_send,input);
            
            // set game state to wait after performing turn
            client->state = GAME_STATE_WAIT;
        }

        // send packet to server
        int res = 0;
        if(buffer_tell(&buffer_send) > 0) res = send(client->socket,buffer_send.buffer,buffer_tell(&buffer_send),0);
        if (res < 0) {
            perror("ERROR sending data to server failed");
            client->terminate = 1;
        }
    }
}

void* network_thread_listener(void* arg){ // thread_listener, thread that performs recv operations
    // get pointer to thread common data, define thread variables and static buffers
    clientdata* client = (clientdata*)arg;
    net_buffer buffer_recv;
    net_buffer buffer_send;
    char string[INPUT_MAX];
    char hin,hout;

    // listen loop
    while(client->terminate == 0){
        // reset i/o buffers and recv buffer from socket
        memset(string, '\0', sizeof(string));
        buffer_seek(&buffer_recv,0);
        int res = recv(client->socket, buffer_recv.buffer, PACKET_MAX, 0);
        buffer_recv.buffer[res] = '\0';
        if(res <= 0){
            if(res < 0) perror("ERROR recv error");
            else printf("ERROR lost connection to server\n");
            close(client->socket);
            client->terminate = 1;
            continue;
        }
        // get header for packet
        hin = *(char*)buffer_read(&buffer_recv,sizeof(char));

        // start packet stream for buffer, as packets can be concantenated for easier use. theyre only a few bytes each so this is great actually, less overhead for send and recv
        while(hin != '\0' && client->terminate == 0){
            // process packet header
            switch(hin){
                case HEADER_TEXT:
                    buffer_read_string(&buffer_recv,string);
                    printf("%s\n",string);
                break;

                case HEADER_MOVE:
                    // client receives this periodically from the server if it is their turn, in case of error that causes the client to forget, or if they somehow send a bad input
                    //char val = *(int*)buffer_read(&buffer_recv,sizeof(char));
                    if(client->state != GAME_STATE_GO) printf("it's your turn.\n");
                    client->state = GAME_STATE_GO;
                break;

                case HEADER_END:
                    // the server has disconnected the client, and told it first
                    printf("you've been disconnected from the server.\n");
                    client->terminate = 1;
                break;

                default:
                    // always an out of bounds header, or malformed packet, meaning server is faulty
                    printf("ERROR malformed packet or bad header from server.\n");
                    client->terminate = 1;
                break;
            }
            // get next header
            hin = *(char*)buffer_read(&buffer_recv,sizeof(char));
        }
    }
    // end of listener thread
}

int main(int argc, char** argv) {
    // client takes 3-4 arguments, the 4th argument being an optional username because i thought that would be fun
    if(argc <= 3){
        printf("ERROR wrong number of arguments:\n <char* hostname> <int port> [char* username]>");
        return 0;
    }

    // disable stdout line buffering because i was having some issues where things were printing out of order across threads
    setbuf(stdout, NULL);
    
    // connection information. socket descriptor, port from command line, hostname from command line translates to IP. could probably work for DNS hostnames too
    struct sockaddr_in server;
    int clientsock, port = atoi(argv[2]);
    struct hostent *host;
    host = gethostbyname(argv[1]);

    char* ipaddr;
    if (host == NULL) {
        printf("ERROR cannot resolve hostname\n");
        return 0;
    }
        
    ipaddr = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);

    // define target -> TCP server at ipaddr:port
    server.sin_addr.s_addr = inet_addr(ipaddr);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // create TCP socket
    clientsock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientsock < 0) {
        perror("ERROR cannot create socket");
        return 0;
    }

    printf("Attempting to connect to %s[%s:%i]... ",argv[1],ipaddr,port);

    // attempt connection
    int res = connect(clientsock, (struct sockaddr *) &server, sizeof(server));
    if (res < 0) {
        printf("ERROR connection failed\n");
        return 0;
    }
    printf("ok!\n");
    printf("you can send text messages at any time. sending 'quit' terminates the connection.\n");

    // initialize client thread common data
    char* user = "userclient";
    clientdata client;
    client.state = GAME_STATE_WAIT;
    client.socket = clientsock;
    client.terminate = 0;
    client.ping = 1;
    strncpy(client.name,argc > 3 ? argv[3] : user,NAME_MAX*sizeof(char));

    // start actor and listener threads
    pthread_t actor,listener;
    pthread_create(&actor, NULL, network_thread_actor, (void*)&client);
    pthread_create(&listener, NULL, network_thread_listener, (void*)&client);

    // wait for listener thread to finish, meaning it receives a bad packet or HEADER_END
    pthread_join(listener, NULL);
    pthread_cancel(actor); // serious hack because i dont want to over complicate the input stream by using anything other than fgets
    //close(clientsock);
    printf("goodbye!\n");
    return 0;
}
