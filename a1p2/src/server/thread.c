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
#include "../protocol.h"
#include "server.h"

void enqueue(cnode* cptr, threadcommon* common) {
    pthread_mutex_lock(&common->lock);
    common->task_queue[common->task_count++] = cptr;
    pthread_cond_signal(&common->cond);
    pthread_mutex_unlock(&common->lock);
}

cnode* dequeue(threadcommon* common) {
    pthread_mutex_lock(&common->lock);
    while (common->task_count == 0) {
        pthread_cond_wait(&common->cond, &common->lock);
    }
    cnode* cptr = common->task_queue[--common->task_count];
    //printf("socket should: %i\n",cptr->data.socket);
    pthread_mutex_unlock(&common->lock);
    return cptr;
}

void* thread_worker(void* arg) {
    threadcommon *common = (threadcommon*)arg; // READ ONLY WITHOUT MUTEX LOCK
    while (1) {
        cnode* cptr = dequeue(common);
        handle_client(common, (void*)cptr);
    }
    return NULL;
}