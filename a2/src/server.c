#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <time.h>
#include <netdb.h>
#include <string.h>
#include <math.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/wait.h>
#include "shmem.h"


typedef struct queue {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    void* buffer[sizeof(u32)*POOL*32];
    u8 size;
    u8 count;
    u32 lim;
} queue;

typedef struct worker {
    u8 id;
    u8 slot;
    u32 value;
} worker;

queue input[POOL];
queue output[POOL];

void queue_list_init(queue* q,u32 lim){
    for(int i = 0; i < lim; i++){
        pthread_mutex_init(&q[i].lock,NULL);
        q[i].count = 0;
        q[i].lim = lim;
        memset(q[i].buffer,0,sizeof(q[i].buffer));
    }
}

void queue_list_destroy(queue* q){
    for(int i = 0; i < POOL; i++) pthread_mutex_destroy(&q[i].lock);
}

void enqueue(queue* q, u8 size, void* i){
    while(q->count >= POOL);
    pthread_mutex_lock(&q->lock);
    memcpy(q->buffer[q->count++],i,size);
    pthread_mutex_unlock(&q->lock);
}

void dequeue(queue* q, u8 size, void* out){
    while(q->count == 0);
    pthread_mutex_lock(&q->lock);
    memcpy(q->buffer[q->count--],out,size);
    pthread_mutex_unlock(&q->lock);
}

void* worker_thread(){
    for(;;){
        worker this;
        dequeue(input,sizeof(worker),&this);

        // trial dvisison

        u32 out = 3456;
        enqueue(output,sizeof(u32),&out);
    }
}

int main(int argc, char** argv){
    struct pollfd fds[1];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    int shm_fd;
    char* shm;
    shm_read* data = (shm_read*)shm_create(&shm_fd,shm,1);

    //queue_list_init(input,POOL*32);
    //queue_list_init(output,POOL);

    // processing
    pthread_t workers[POOL*32];
    u8 count = 0;

    // do

    for(;;){
        // supposedly shmem isnt released at program close so i have a graceful exit on server just in case
        int ret = poll(fds,1,0);
        if(ret > 0 && fds[0].revents & POLLIN){
            // receive input
            u8 buffer_input[INPUT];
            fgets(buffer_input,sizeof(buffer_input), stdin);
            if(strncmp(buffer_input,"quit",4) == 0) break;
        }



        //sem_wait(data->sem);
        pthread_mutex_lock(&data->lock);

        /*printf("\033[s");
        printf("\033[5A");
        printf("\033[K");
        printf("clien f:%d s:%d c:%i",data->clientflag,data->clientslot,clock());
        printf("\033[u");*/

        while(count >= POOL);
        if(data->clientflag == 1){
            data->clientflag = count;
            data->flag[count] = 1;
            u32 process = data->clientslot;
            printf("POP! %u\n",process);
            for(int i = 0; i < 32; i++){
                u32 shifted = (process>>i)|(process<<(32-i));
                printf("%u\n",shifted);
            }
        }

        pthread_mutex_unlock(&data->lock);
        usleep(100);
        //sem_post(data->sem);
    }

    //queue_list_destroy(input);
    //queue_list_destroy(output);
    
    shm_destroy(&shm_fd,shm,1);
    return 0;
}