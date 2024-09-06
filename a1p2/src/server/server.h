#include "../protocol.h"
#include <pthread.h>

//#define PORT 8080
#define BACKLOG 10
#define BUF_SIZE 1024
#define MAXLINE 30

#define THREAD_POOL_SIZE 5

typedef struct cnode {
    clientdata data;
    struct cnode *next, *last;
} cnode;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;

    struct sockaddr_in server;
    int serversock;

    cnode* task_queue[BACKLOG];
    int task_count;
    int all_terminate;

    int socketcount;
    //clientdata sockets[16];
    cnode* sockets;

    int turn,val,state,min,valdef;
} threadcommon;

void client_insert(threadcommon* common);
void client_remove(threadcommon* common,cnode* node);
void client_rotate(threadcommon* common);
int client_count(threadcommon* common);
//int client_find(threadcommon* common,cnode* node);

void* handle_client(threadcommon* common, void* arg);

void enqueue(cnode* cptr, threadcommon* common);
cnode* dequeue(threadcommon* common);
void* thread_worker(void* arg);

void network_disconnect(cnode* client);
void network_disconnect_all(threadcommon* common);
int send_all(threadcommon* common, void* buffer, int size, int target, int rule);

void* network_thread_actor(void* arg);
void* network_thread_listener(void* arg);