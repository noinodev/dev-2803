
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
#include "protocol.h"

#define PORT 8080
#define BUF_SIZE 1024

typedef struct {
    int state,socket,terminate;
} clientdata;

void* network_thread_actor(void* arg){
    clientdata* client = (clientdata*)arg;
    char input[INPUT_MAX];
    void* buffer_send[PACKET_MAX];
    while (client->terminate == 0) {
        memset(buffer_send, '\0', sizeof(buffer_send));
        //printf("> ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;

        char *errptr;
        int out = (int)strtod(input,&errptr);
        if(*errptr != '\0' || client->state == GAME_STATE_WAIT){ // user can send text messages if it is not their turn or if they are waiting
            if(strcmp(input, "quit") == 0){
                buffer_send[0] = (void*)HEADER_END;
                //printf("try quit\n");
                client->terminate = 1;
                break;
            }else{
                buffer_send[0] = (void*)HEADER_TEXT;
                //buffer_send+sizeof(int) = (void*)input;
                memcpy(buffer_send+sizeof(uint8_t), input, strlen(input)+1);
                //printf("try text\n");
            }
        }else{
            packet_move packet;
            packet.header = HEADER_MOVE;
            packet.move = (int)fmax(fmin(out,9),1);
            if(packet.move != out) printf("%i is out of bounds. im not going to break anything but i will clamp it to %i for you\n",out,packet.move);
            memcpy(buffer_send,(void*)&packet,sizeof(packet));
            client->state = GAME_STATE_WAIT;
        }
        
        int res = send(client->socket, buffer_send, PACKET_MAX, 0);
        if (res < 0) {
            printf("Sending data to server failed");
            exit(1);
        }
    }
    printf("terminating actor. goodbye!");
}

void* network_thread_listener(void* arg){
    clientdata* client = (clientdata*)arg;
    void* buffer_recv[PACKET_MAX];
    int packetcount = 0;
    while(client->terminate == 0){
        memset(buffer_recv, '\0', sizeof(buffer_recv));
        int res = recv(client->socket, buffer_recv, PACKET_MAX-1, 0);
        packetcount++;
        if(res == 0){
            printf("lost connection with server\n");
            client->terminate = 1;
        }
        buffer_recv[res-sizeof(char)] = '\0';
        uint8_t *header = (uint8_t*)buffer_recv;
        if(*header == HEADER_TEXT){
            char* txt = (char*)(buffer_recv+sizeof(uint8_t));
            printf("%s\n",txt);
        }else if(*header == HEADER_MOVE){
            packet_move* packet_recv = (packet_move*)buffer_recv;
            client->state = GAME_STATE_GO;
            printf("It's your turn. Current value is %i\n",packet_recv->move);
        }else if(*header == HEADER_END){
            printf("You've been disconnected from the server.\n");
            client->terminate = 1;
            break;
        }
    }
    printf("terminating listener. actor will be stopped\n");
}

int main() {
    void* buffer_send[PACKET_MAX];
    void* buffer_recv[PACKET_MAX];
    
    struct sockaddr_in server;  // server address
    int clientsock;             // client socket descriptor
    
    int res;

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    // create socket
    clientsock = socket(AF_INET, SOCK_STREAM, 0);

    if (clientsock < 0) {
        perror("Could not create socket");
    }
    printf("Socket created\n");

    // Connect (to remote server)
    res = connect(clientsock, (struct sockaddr *) &server, sizeof(server));
    if (res == -1) {
        printf("Connection failed\n");
        exit(1);
    }

    int state = GAME_STATE_WAIT;
    clientdata client;
    client.state = GAME_STATE_WAIT;
    client.socket = clientsock;
    client.terminate = 0;

    pthread_t actor,listener;
    pthread_create(&actor, NULL, network_thread_actor, (void*)&client);
    pthread_create(&listener, NULL, network_thread_listener, (void*)&client);

    pthread_join(listener, NULL);
    pthread_cancel(actor); // serious hack because i dont want to over complicate the input stream by using anything other than fgets
    close(clientsock);
    return 0;
}
