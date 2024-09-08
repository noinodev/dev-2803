#include "../protocol.h"
#include <pthread.h>
#include <netinet/in.h>

//#define PORT 8080
#define BACKLOG 10
//#define BUF_SIZE 1024
#define MAXLINE 30

#define THREAD_POOL_SIZE 5

typedef struct threadcommon threadcommon;
typedef struct cnode cnode;
typedef struct gamedata gamedata;

// game struct, just used for 'numbers' for now
typedef struct gamedata {
    int (*handle_move_check)(threadcommon*, char*);
    int (*handle_move_update)(threadcommon*, char*);
    //void (*handle_set_defaults)(threadcommon* common);
    void (*handle_set_reset)(threadcommon*);
    char data[64], def[64], name[NAME_MAX];
} gamedata;

// client node doubly linked list
typedef struct cnode {
    clientdata data;
    struct cnode *next, *last;
} cnode;

// thread shared data because i dont like global scoped variables for some reason
typedef struct threadcommon{
    pthread_mutex_t lock;
    pthread_cond_t cond;

    // address of server, not really used
    struct sockaddr_in server;
    int serversock;

    // thread pool socket queue
    cnode* task_queue[BACKLOG];
    int task_count;
    int all_terminate;

    // root node of cnode* linked list
    cnode* clients;
    gamedata game;

    // game state and turn
    int turn,state;
} threadcommon;

// games function declarations
int game_numbers_move_check(threadcommon* common, char* string);
int game_numbers_move_update(threadcommon* common, char* string);

int game_rps_move_check(threadcommon* common, char* string);
int game_rps_move_update(threadcommon* common, char* string);

void game_reset(threadcommon* common);

// client node linked list helper functions
void client_insert(threadcommon* common);
void client_remove(threadcommon* common,cnode* node);
void client_rotate(threadcommon* common);
int client_count(threadcommon* common);

// client thread function used in thread pool
void* handle_client(threadcommon* common, void* arg);

// thread pool socket queueing, thread workers wait for sockets (cnode*) to be enqueued
void enqueue(cnode* cptr, threadcommon* common);
cnode* dequeue(threadcommon* common);
void* thread_worker(void* arg);

// expanded network functions
void network_disconnect(cnode* client);
void network_disconnect_all(threadcommon* common);
int send_all(threadcommon* common, void* buffer, int size, int target, int rule);

// server actor/listener thread functions
void* network_thread_actor(void* arg);
void* network_thread_listener(void* arg);