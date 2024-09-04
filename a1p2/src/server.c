#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/utsname.h>
#include "protocol.h"

#define PORT 8080
#define BACKLOG 10
#define BUF_SIZE 1024
#define MAXLINE 30

#define THREAD_POOL_SIZE 5

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;

    int task_queue[BACKLOG];
    int task_count;
    int all_terminate;

    int socketcount;
    int sockets[16];

    int turn, val,state;
} threadcommon;

typedef struct {
    int socket;
} threadprivate;

typedef struct {
    int turn, val;
} state;

void* handle_client(threadcommon* common, void* arg);

void enqueue(int clientsock, threadcommon* common) {
    pthread_mutex_lock(&common->lock);
    common->task_queue[common->task_count++] = clientsock;
    pthread_cond_signal(&common->cond);
    pthread_mutex_unlock(&common->lock);
}

int dequeue(threadcommon* common) {
    pthread_mutex_lock(&common->lock);
    while (common->task_count == 0) {
        pthread_cond_wait(&common->cond, &common->lock);
    }
    int clientsock = common->task_queue[--common->task_count];
    pthread_mutex_unlock(&common->lock);
    return clientsock;
}

void* thread_worker(void* arg) {
    threadcommon *common = (threadcommon*)arg; // READ ONLY WITHOUT MUTEX LOCK
    //threadprivate *private;
    while (1) {
        int socket = dequeue(common);
        handle_client(common, (void*)&socket);
    }
    return NULL;
}

int send_all(threadcommon* common, void* buffer, int target, int rule){
    for(int i = 0; i < common->socketcount; i++){
        int socket = common->sockets[i], is_in_target = 0;
        if(socket == target && rule == NETWORK_TARGET_TO) is_in_target = 1;
        if(socket != target && rule == NETWORK_TARGET_EXCEPT) is_in_target = 1;
        if(rule == NETWORK_TARGET_ALL) is_in_target = 1;

        if(is_in_target == 1){
            printf("send packet from %i to socket[%i]=%i : %s\n",socket,i,target,(char*)buffer);
            send(socket,buffer,PACKET_MAX,0);
        }
    }
}

void* handle_client(threadcommon* common, void* arg) {
    int socket = *((int*)arg);
    void* buffer_send[PACKET_MAX];
    void* buffer_recv[PACKET_MAX];
    int read_size, thread_terminate = 0;

    printf("client thread is active\n");

    pthread_mutex_lock(&common->lock);
    if(common->state == GAME_STATE_WAIT){
        packet_move packet_send;
        packet_send.header = HEADER_MOVE;
        packet_send.move = common->val;
        memcpy(buffer_send,(void*)&packet_send,sizeof(packet_send));
        //network_send_raw(common,buffer_send,socket,NETWORK_TARGET_TO);
        send(socket,buffer_send,PACKET_MAX,0);
        common->state = GAME_STATE_GO;
    }
    pthread_mutex_unlock(&common->lock);

    while(common->all_terminate | thread_terminate == 0){
        memset(buffer_send, '\0', sizeof(buffer_send));
        memset(buffer_recv, '\0', sizeof(buffer_recv));
        int res = recv(socket, buffer_recv, PACKET_MAX-1, 0);
        if(res < 0){
            perror("recv failed");
            break;
        }else if(res == 0){
            printf("disconnect\n");
            break;
        }
        buffer_recv[res-sizeof(char)] = '\0';
        uint8_t *header = (uint8_t*)buffer_recv;
        printf("header %i\n",*header);
        if(*header == HEADER_TEXT){
            char* txt = (char*)(buffer_recv+sizeof(uint8_t));
            printf("client says %s\n",txt);
            buffer_send[0] = (void*)HEADER_TEXT;
            memcpy(buffer_send+sizeof(uint8_t), txt, strlen(txt)+1);
            send_all(common,buffer_send,socket,NETWORK_TARGET_EXCEPT);
        }else if(*header == HEADER_MOVE){
            pthread_mutex_lock(&common->lock);
            packet_move* packet_recv = (packet_move*)buffer_recv;
            common->val -= packet_recv->move;
            printf("state value now %i\n",common->val);

            if(common->val > 0){
                packet_move packet_send;
                packet_send.header = HEADER_MOVE;
                packet_send.move = common->val;
                memcpy(buffer_send,(void*)&packet_send,sizeof(packet_send));
                send(common->sockets[(common->turn++)%common->socketcount],buffer_send,PACKET_MAX,0);
            }else{
                char* txt;
                txt = "you win :)";
                buffer_send[0] = (void*)HEADER_TEXT;
                memcpy(buffer_send+sizeof(uint8_t), txt, strlen(txt)+1);
                send(socket,buffer_send,PACKET_MAX,0);

                txt = "you lose :(";
                buffer_send[0] = (void*)HEADER_TEXT;
                memcpy(buffer_send+sizeof(uint8_t), txt, strlen(txt)+1);
                send_all(common,buffer_send,socket,NETWORK_TARGET_EXCEPT);

                common->all_terminate = 1;
            }
            pthread_mutex_unlock(&common->lock);
            //network_send_raw(common,buffer_send,socket,NETWORK_TARGET_TO);
        }else if(*header == HEADER_END){
            printf("client say byebye\n");
            break;
        }else{
            printf("client )
        }
    }

    printf("terminating thread process\n");

    if (read_size == 0) {
        printf("Client disconnected\n");
    } else if (read_size == -1) {
        perror("recv failed");
    }

    close(socket);
    return NULL;
}

int main() {
    void* buffer_send[PACKET_MAX];
    pthread_t thread_pool[THREAD_POOL_SIZE];

    struct sockaddr_in server, client;
    int res;
    int readSize;

    int serversock, clientsock;

    threadcommon common;
    common.lock = PTHREAD_MUTEX_INITIALIZER;
    common.cond = PTHREAD_COND_INITIALIZER;
    common.task_count = 0;
    common.all_terminate = 0;
    common.socketcount = 0;
    common.turn = 0;
    common.val = 25;
    common.state = 0;
    
    // Create TCP socket
    serversock = socket(AF_INET, SOCK_STREAM, 0);
    if (serversock == -1) {
        printf("Creating socket failed\n");
        exit(1);
    }
    printf("Socket successfully created\n");
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
    
    res = bind(serversock, (struct sockaddr *) &server, sizeof(server));
    if(res < 0){
        printf("Bind failed\n");
        exit(1);
    }
    printf("Bind was successfully completed\n");
    
    res = listen(serversock, BACKLOG);
    if(res != 0){
        printf("Listen failed\n");
        exit(1);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) pthread_create(&thread_pool[i], NULL, thread_worker, &common);
    printf("Server is listening on port %d\n", PORT);

    // Accept connections
    while (1) {
        int clientlen = sizeof(client);
        clientsock = accept(serversock, (struct sockaddr*)&client, &clientlen);
        if (clientsock < 0) {
            perror("Accept failed");
            continue;
        }
        common.sockets[common.socketcount++] = clientsock;

        printf("socket accept\n");

        // Enqueue the client socket
        enqueue(clientsock,&common);
    }
    close(clientsock);
    close(serversock);
    return 0;
}
