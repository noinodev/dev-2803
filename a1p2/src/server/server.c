#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <time.h>
#include <netdb.h>
#include "../protocol.h"
#include "server.h"


// insert a node into the start of the linked list
void client_insert(threadcommon* common){
    cnode* node = (cnode*)malloc(sizeof(cnode));
    node->next = common->sockets;
    node->last = NULL;
    common->sockets = node;
}

// remove a node from the linked list
void client_remove(threadcommon* common,cnode* node){
    if(node->next != NULL) node->next->last = node->last;
    if(node->last != NULL) node->last->next = node->next;
    else common->sockets = node->next;
    free(node);
    node = NULL;
}

// move root node of linked list to the end
void client_rotate(threadcommon* common){
    cnode* root = common->sockets;
    if(root->next == NULL) return;
    common->sockets = root->next;
    common->sockets->last = NULL;

    cnode* end = root->next;
    while(end->next != NULL) end = end->next;

    end->next = root;
    root->next = NULL;
    root->last = end;
}

// count nodes in linked list
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

// send HEADER_END on smallest possible packet size to client, and flag its thread for termination
void network_disconnect(cnode* node){
    char buffer[PACKET_MIN];
    buffer[0] = HEADER_END;
    send(node->data.socket,buffer,sizeof(char),0);
    node->data.terminate = 1;
}

// the same thing but for all clients in the common->sockets linked list
void network_disconnect_all(threadcommon* common){
    cnode* node = common->sockets;
    char buffer[PACKET_MIN];
    buffer[0] = HEADER_END;
    while(node != NULL){
        send(node->data.socket,buffer,sizeof(char),0);
        node->data.terminate = 1;
        node = node->next;
    }
}

// send a packet to all sockets in cnode*, but also with rules so client threads dont send things to themselves, or if a linear search needs to be done to send to that client (rare case lol)
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
    // server takes 4 arguments, for port, type of game, number of players before it starts (default 2), and the number that the numbers game starts with (default 25)
    if(argc <= 3){
        printf("wrong number of arguments:\n<int port> <char* game type> <int minimum users*> <int starting number* >");
        return 0;
    }

    // resolve own hostname for debug, define post
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("Local Machine Name: %s\n", hostname);
    int port = atoi(argv[1]);

    // relevant threads -> thread pool for clients, actor/listener for server
    pthread_t thread_pool[THREAD_POOL_SIZE];
    pthread_t actor, listener;

    // disable stdout line buffer, because i was having issues (see client.c also)
    setbuf(stdout, NULL);

    // initialize data common between threads because i dont know what a pipe is and i dont want to use global variables ewww
    threadcommon common;
    common.lock = PTHREAD_MUTEX_INITIALIZER;
    common.cond = PTHREAD_COND_INITIALIZER;
    common.task_count = 0;
    common.all_terminate = 0;
    common.turn = 0;
    common.valdef = argc >= 5 ? atoi(argv[4]) : 25;
    common.val = common.valdef;
    common.min = argc >= 4 ? atoi(argv[3]) : 2;
    common.state = GAME_STATE_WAIT;
    common.sockets = NULL;
    
    // create TCP socket
    common.serversock = socket(AF_INET, SOCK_STREAM, 0);
    if (common.serversock == -1) {
        printf("Creating socket failed\n");
        exit(1);
    }
    
    // define TCP server address
    common.server.sin_family = AF_INET;
    common.server.sin_addr.s_addr = INADDR_ANY;
    common.server.sin_port = htons(port);
    
    // bind socket to server
    int res = bind(common.serversock, (struct sockaddr *) &common.server, sizeof(common.server));
    if(res < 0){
        printf("Bind failed\n");
        exit(1);
    }
    
    // start server listening
    res = listen(common.serversock, BACKLOG);
    if(res != 0){
        printf("Listen failed\n");
        exit(1);
    }

    // run threads
    for (int i = 0; i < THREAD_POOL_SIZE; i++) pthread_create(&thread_pool[i], NULL, thread_worker, &common);
    pthread_create(&actor, NULL, network_thread_actor, &common);
    pthread_create(&actor, NULL, network_thread_listener, &common);
    printf("Server is listening on port %d\n", port);

    // join threads and exit
    pthread_join(actor,NULL);
    pthread_join(listener,NULL);
    return 0;
}
