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
#include <math.h>
#include <time.h>
#include <netdb.h>
#include "../protocol.h"
#include "server.h"


void client_insert(threadcommon* common){
    cnode* node = (cnode*)malloc(sizeof(cnode));
    //c->data = *data;
    node->next = common->sockets;
    node->last = NULL;
    //return node;
    common->sockets = node;
}

void client_remove(threadcommon* common,cnode* node){
    //cnode* root = common->sockets;
    //if(client_find(common,node) == 0) return;
    if(node->next != NULL) node->next->last = node->last;
    if(node->last != NULL) node->last->next = node->next;
    else common->sockets = node->next;
    //free(node->data);
    free(node);
    node = NULL;
}

void client_rotate(threadcommon* common/*, cnode* root*/){
    cnode* root = common->sockets;
    if(root->next == NULL) return;
    common->sockets = root->next;
    common->sockets->last = NULL;

    cnode* end = root->next;
    while(end->next != NULL) end = end->next;

    end->next = root;
    root->next = NULL;
    root->last = end;

    //n->next = root;
    //root->next->last = NULL;
    //root->last = n;
    //cnode* out = root->next;
}

int client_count(threadcommon* common){
    int count = 0;
    cnode* node = common->sockets;
    while(node != NULL){
        //printf("[%i->%i] ",node->data.socket);
        count++;
        node = node->next;
    }
    return count;
}

/*int client_find(threadcommon* common,cnode* nodesrc){
    cnode* node = common->sockets;
    while(node != NULL){
        if(node == nodesrc) return 1;
        node = node->next;
    }
    return 0;
}*/

void network_disconnect(cnode* node){
    char buffer[PACKET_MIN];
    buffer[0] = HEADER_END;
    send(node->data.socket,buffer,sizeof(char),0);
    //printf("forcing disconnect\n");
    //printf("send disconnect: %i\n",send(node->data.socket,buffer,sizeof(char),0));
    node->data.terminate = 1;
    //close(node->data.socket);
    //node->data.socket = -1;
    //client_remove()
}

void network_disconnect_all(threadcommon* common){
    //printf("DISCONNECT ALL!\n");
    cnode* node = common->sockets;
    char buffer[PACKET_MIN];
    buffer[0] = HEADER_END;
    //send(node->data.socket,buffer,sizeof(char),0);
    while(node != NULL){
        send(node->data.socket,buffer,sizeof(char),0);
        node->data.terminate = 1;
        node = node->next;
    }
}

int send_all(threadcommon* common, void* buffer, int size, int target, int rule){
    cnode *node = common->sockets;
    int count = 0;
    while(node != NULL){
        int is_in_target = 0, socket = node->data.socket;
        if(rule == NETWORK_TARGET_ALL) is_in_target = 1;
        else if(socket == target && rule == NETWORK_TARGET_TO) is_in_target = 1;
        else if(socket != target && rule == NETWORK_TARGET_EXCEPT) is_in_target = 1;

        if(is_in_target == 1) count +=  send(socket,buffer,size,0) > 0 ? 1 : 0;
        node = node->next;
    }
    return count;
}

int main(int argc, char** argv) {
    if(argc <= 3){
        printf("wrong number of arguments:\n<int port> <char* game type> <int minimum users*> <int starting number* >");
        return 0;
    }

    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("Local Machine Name: %s\n", hostname);


    int port = atoi(argv[1]);

    //void* buffer_send[PACKET_MAX];
    pthread_t thread_pool[THREAD_POOL_SIZE];
    pthread_t actor, listener;

    setbuf(stdout, NULL);

    //struct sockaddr_in server, client;
    int res;
    int readSize;

    //int serversock, clientsock;

    threadcommon common;
    common.lock = PTHREAD_MUTEX_INITIALIZER;
    common.cond = PTHREAD_COND_INITIALIZER;
    common.task_count = 0;
    common.all_terminate = 0;
    common.socketcount = 0;
    common.turn = 0;
    common.valdef = argc >= 5 ? atoi(argv[4]) : 25;
    common.val = common.valdef;
    common.min = argc >= 4 ? atoi(argv[3]) : 2;
    common.state = GAME_STATE_WAIT;
    common.sockets = NULL;
    
    // Create TCP socket
    common.serversock = socket(AF_INET, SOCK_STREAM, 0);
    if (common.serversock == -1) {
        printf("Creating socket failed\n");
        exit(1);
    }
    printf("Socket successfully created\n");
    
    common.server.sin_family = AF_INET;
    common.server.sin_addr.s_addr = INADDR_ANY;
    common.server.sin_port = htons(port);
    
    res = bind(common.serversock, (struct sockaddr *) &common.server, sizeof(common.server));
    if(res < 0){
        printf("Bind failed\n");
        exit(1);
    }
    //printf("Bind was successfully completed\n");
    
    res = listen(common.serversock, BACKLOG);
    if(res != 0){
        printf("Listen failed\n");
        exit(1);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) pthread_create(&thread_pool[i], NULL, thread_worker, &common);
    pthread_create(&actor, NULL, network_thread_actor, &common);
    pthread_create(&actor, NULL, network_thread_listener, &common);
    //pthread_create(&listener, NULL, network_thread_listener, &common);
    printf("Server is listening on port %d\n", port);

    //int time = clock();

    // Accept connections
    pthread_join(actor,NULL);
    pthread_join(listener,NULL);
    //close(serversock);
    return 0;
}
