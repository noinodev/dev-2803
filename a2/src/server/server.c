#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <time.h>
#include <netdb.h>
#include <string.h>
#include <math.h>
#include "../protocol.h"
#include "server.h"

// insert a node into the start of the linked list
void client_insert(threadcommon* common){
    cnode* node = (cnode*)malloc(sizeof(cnode));
    node->next = common->clients;
    node->last = NULL;
    common->clients = node;
}

// remove a node from the linked list
void client_remove(threadcommon* common,cnode* node){
    if(node->next != NULL) node->next->last = node->last;
    if(node->last != NULL) node->last->next = node->next;
    else common->clients = node->next;
    free(node);
    node = NULL;
}

// move root node of linked list to the end
void client_rotate(threadcommon* common){
    cnode* root = common->clients;
    if(root->next == NULL) return;
    common->clients = root->next;
    common->clients->last = NULL;

    cnode* end = root->next;
    while(end->next != NULL) end = end->next;

    end->next = root;
    root->next = NULL;
    root->last = end;
}

// count nodes in linked list
int client_count(threadcommon* common){
    int count = 0;
    cnode* node = common->clients;
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

// the same thing but for all clients in the common->clients linked list
void network_disconnect_all(threadcommon* common){
    cnode* node = common->clients;
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
    cnode *node = common->clients;
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
        printf("wrong number of arguments:\n<int port> <char* game type> <game args>...");
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

    // disable stdout line buffering, because i was having issues (see client.c also)
    setbuf(stdout, NULL);

    // initialize data common between threads because i dont know what a pipe is and i dont want to use global variables ewww
    threadcommon common;
    common.lock = PTHREAD_MUTEX_INITIALIZER; // i didnt use these as much as i should
    common.cond = PTHREAD_COND_INITIALIZER; // race conditions are pretty unlikely in this program anyway, from what ive tested. the threads are blocked 99% of the time and probably never do anything simultaneously
    common.task_count = 0;
    common.all_terminate = 0;
    common.turn = 0;
    common.state = GAME_STATE_WAIT;
    common.clients = NULL;

    // game function pointers
    common.game.handle_move_check = NULL;
    common.game.handle_move_update = NULL;
    strncpy(common.game.name,argv[2],NAME_MAX);

    // set game function pointers
    if(strcmp(argv[2],"numbers") == 0){
        common.game.handle_move_check = game_numbers_move_check;
        common.game.handle_move_update = game_numbers_move_update;
        common.game.def[0] = argc > 3 ? atoi(argv[3]) : 2; // minimum players for numbers
        common.game.def[1] = argc > 4 ? atoi(argv[4]) : 25; // starting value for game
        common.game.def[2] = '\0';
        game_reset(&common);
    }else if(strcmp(argv[2],"rps") == 0){ // other games because this was really easy
        common.game.handle_move_check = game_rps_move_check;
        common.game.handle_move_update = game_rps_move_update;
        common.game.def[0] = argc > 3 ? atoi(argv[3]) : 2; // minimum players for rock paper scissors
        common.game.def[1] = 0;
        common.game.def[2] = 0;
        common.game.def[3] = '\0';
        game_reset(&common);
    }
    
    // create TCP socket
    common.serversock = socket(AF_INET, SOCK_STREAM, 0);
    if (common.serversock < 0) {
        printf("Creating socket failed\n");
        exit(1);
    }else printf("Socket ok!\n");
    
    // define TCP server address
    common.server.sin_family = AF_INET;
    common.server.sin_addr.s_addr = INADDR_ANY;
    common.server.sin_port = htons(port);
    
    // bind socket to server
    int res = bind(common.serversock, (struct sockaddr *) &common.server, sizeof(common.server));
    if(res < 0){
        printf("Bind failed\n");
        exit(1);
    }else printf("Bind ok!\n");
    
    // start server listening
    res = listen(common.serversock, BACKLOG);
    if(res < 0){
        printf("Listen failed\n");
        exit(1);
    }else printf("Listen ok!\n");

    // run threads
    pthread_create(&listener, NULL, network_thread_listener, &common);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) pthread_create(&thread_pool[i], NULL, thread_worker, &common);
    pthread_create(&actor, NULL, network_thread_actor, &common);
    printf("Server is listening on port %d\n", port);

    // join threads and exit
    for (int i = 0; i < THREAD_POOL_SIZE; i++) pthread_join(thread_pool[i], NULL);
    pthread_join(actor,NULL);
    pthread_join(listener,NULL);
    return 0;
}
