
// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <netdb.h>
#include "protocol.h"

//#define PORT 8080
//#define BUF_SIZE 1024

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void* network_thread_actor(void* arg){
    clientdata* client = (clientdata*)arg;
    char input[INPUT_MAX];
    char hout;
    //char buffer_send[PACKET_MAX];
    net_buffer buffer_send = buffer_create();

    buffer_seek(&buffer_send,0);
    hout = HEADER_INFO;
    buffer_write(&buffer_send,&hout,sizeof(char));
    buffer_write_string(&buffer_send,client->name);
    //buffer_send[0] = HEADER_INFO;
    //memcpy(buffer_send+sizeof(char), client->name, strlen(client->name)+1);
    send(client->socket,buffer_send.buffer,buffer_tell(&buffer_send),0);

    while (client->terminate == 0) {
        //memset(buffer_send, '\0', sizeof(buffer_send));
        buffer_seek(&buffer_send,0);
        memset(input, '\0', sizeof(input));
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';
        //printf("entered: %s)

        char *errptr;
        int out = (int)strtod(input,&errptr);
        if(*errptr != '\0' || client->state == GAME_STATE_WAIT){ // user can send text messages if it is not their turn or if they are waiting
            if(strcmp(input, "quit") == 0){
                hout = HEADER_END;
                buffer_write(&buffer_send,&hout,sizeof(char));
                //buffer_send[0] = (char)HEADER_END;
                client->terminate = 1;
            }else{
                //buffer_send[0] = (char)HEADER_TEXT;
                //memcpy(buffer_send+sizeof(char), input, strlen(input)+1);
                hout = HEADER_TEXT;
                buffer_write(&buffer_send,&hout,sizeof(char));
                buffer_write_string(&buffer_send,input);
            }
        }else{
            char move = (char)fmax(fmin(out,9),1);
            //if(move != out) printf("%i is out of bounds. im not going to break anything but i will clamp it to %i for you\n",out,move);
            //memcpy(buffer_send,(char*)&packet,sizeof(packet));
            //buffer_send[0] = (char)HEADER_MOVE;
            //buffer_send[1] = (char)out;
            hout = HEADER_MOVE;
            buffer_write(&buffer_send,&hout,sizeof(char));
            buffer_write(&buffer_send,&move,sizeof(char));
            
            pthread_mutex_lock(&lock);
            client->state = GAME_STATE_WAIT;
            pthread_mutex_unlock(&lock);
        }

        //hout = HEADER_PING;
        //buffer_write(&buffer_send,&hout,sizeof(char));

        int res;
        if(buffer_tell(&buffer_send) > 0) res = send(client->socket,buffer_send.buffer,buffer_tell(&buffer_send),0);
        if (res < 0) {
            printf("ERROR sending data to server failed");
            exit(1);
        }

        /*int res;
        if(buffer_send[0] > 0 && buffer_send[0] <= HEADER_INFO) res = send(client->socket, buffer_send, PACKET_MAX, 0);
        if (res < 0) {
            printf("Sending data to server failed");
            exit(1);
        }*/
    }
    //printf("terminating actor. goodbye!");
}

void* network_thread_listener(void* arg){
    clientdata* client = (clientdata*)arg;
    //char buffer_recv[PACKET_MAX];
    net_buffer buffer_recv = buffer_create();
    net_buffer buffer_send = buffer_create();
    char string[INPUT_MAX];
    char hin,hout;

    while(client->terminate == 0){
        memset(string, '\0', sizeof(string));
        buffer_seek(&buffer_recv,0);
        int res = recv(client->socket, buffer_recv.buffer, PACKET_MAX, 0);
        buffer_recv.buffer[res] = '\0';
        if(res <= 0){
            printf("ERROR lost connection to server\n");
            client->terminate = 1;
        }//else printf("recv something '%i'",res);
        //hin = *(char*)buffer_read(&buffer_recv,sizeof(char));

        hin = *(char*)buffer_read(&buffer_recv,sizeof(char));
        // PACKET STREAMING HOLY MOLY
        int pktc = 0;
        //pthread_mutex_lock(&lock);
        while(hin != '\0' && client->terminate == 0){
            pktc++;
            switch(hin){
                case HEADER_TEXT:
                    buffer_read_string(&buffer_recv,string);
                    printf("%s\n",string);
                break;

                case HEADER_MOVE:
                    char val = *(int*)buffer_read(&buffer_recv,sizeof(char));
                    //pthread_mutex_lock(&lock);
                    if(client->state != GAME_STATE_GO) printf("it's your turn. current value is %i. enter a number 0->9.\n",val);
                    client->state = GAME_STATE_GO;
                    //pthread_mutex_unlock(&lock);
                break;

                case HEADER_END:
                    printf("you've been disconnected from the server.\n");
                    client->terminate = 1;
                break;

                /*case HEADER_INFO:
                    int state = *(int*)buffer_read(&buffer_recv,sizeof(char));
                    switch(state){
                        case GAME_STATE_GO:
                            printf("Game is starting.\n");
                        break;
                        case GAME_STATE_WAIT:
                            printf("Waiting for players\n");
                        break;
                    }
                break;*/

                default:
                    printf("ERROR malformed packet or bad header from server.\n");
                    client->terminate = 1;
                    pktc--;
                break;
            }
            hin = *(char*)buffer_read(&buffer_recv,sizeof(char));
        }
        //printf("finished packet handling, %i valid packets in stream\n",pktc);
        //pthread_mutex_unlock(&lock);
    }
    //printf("terminating listener.\n");
}

int main(int argc, char** argv) {
    //char buffer_send[PACKET_MAX];
    //char buffer_recv[PACKET_MAX];
    if(argc <= 3){
        printf("ERROR wrong number of arguments:\n<char* gametype> <char* hostname> <int port> [char* username]>");
        return 0;
    }

    setbuf(stdout, NULL);
    
    struct sockaddr_in server;  // server address
    int clientsock;             // client socket descriptor
    
    int res;

    int port = atoi(argv[3]);

    struct hostent *host;
    host = gethostbyname(argv[2]);

    if (host == NULL) {
        printf("ERROR cannot resolve hostname\n");
        return 0;
    }

    char* ipaddr = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
    //printf("%s resolves to %s",argv[2],ipaddr);

    server.sin_addr.s_addr = inet_addr(ipaddr);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // create socket
    clientsock = socket(AF_INET, SOCK_STREAM, 0);

    if (clientsock < 0) {
        perror("ERROR cannot create socket");
        return 0;
    }
    printf("Attempting to connect to %s[%s:%i]... ",argv[2],ipaddr,port);

    // Connect (to remote server)
    res = connect(clientsock, (struct sockaddr *) &server, sizeof(server));
    if (res == -1) {
        printf("ERROR connection failed\n");
        return 0;
    }
    printf("ok!\n");
    printf("you can send text messages at any time. sending 'quit' terminates the connection.\n");

    char* user = "user";
    //int state = GAME_STATE_WAIT;
    clientdata client;
    client.state = GAME_STATE_WAIT;
    client.socket = clientsock;
    client.terminate = 0;
    client.ping = 1;
    //memcpy(client.name, argv[1], strlen(argv[1])+1);
    strncpy(client.name,argc > 4 ? argv[4] : user,sizeof(client.name));
    strncpy(client.type,argc > 4 ? argv[1] : user,sizeof(client.name));

    //client.name = *argv[1];

    pthread_t actor,listener;
    pthread_create(&actor, NULL, network_thread_actor, (void*)&client);
    pthread_create(&listener, NULL, network_thread_listener, (void*)&client);

    pthread_join(listener, NULL);
    pthread_cancel(actor); // serious hack because i dont want to over complicate the input stream by using anything other than fgets
    close(clientsock);
    printf("goodbye!\n");
    return 0;
}
