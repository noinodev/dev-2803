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
#include <sys/sem.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "shmem.h"

// Task struct for queue
struct Task;
typedef struct Task {
    u8 slot;
    u8 offset;
    u32 value;
} Task;

// Circular queue
typedef struct {
    Task tasks[4*SHIFTS*POOL];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} TaskQueue;

TaskQueue input;
u8 status[SHIFTS*POOL];
//TaskQueue output;
//double progress[POOL];

void queue_list_init(TaskQueue* q){
    pthread_mutex_init(&q->lock,NULL);
    pthread_cond_init(&q->cond,NULL);
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    memset(q->tasks,0,sizeof(q->tasks));
}

void queue_list_destroy(TaskQueue* q){
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);
}

void enqueue(TaskQueue* q, size_t size, Task* i){
    //while(q->count >= POOL){usleep(100);}
    while (q->count >= sizeof(q->tasks) / sizeof(Task)) pthread_cond_wait(&q->cond, &q->lock);
    pthread_mutex_lock(&q->lock);
    //printf("cpy...");
    memcpy(&q->tasks[q->count++],i,size);
    //printf("signal...");
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
    //printf("ok!\n");
    //printf("EQ: s:%i, %u/%u\n",i->slot, q->tasks[q->count-1].value, i->value);
    //usleep(1);
}

void dequeue(TaskQueue* q, size_t size, Task* out){
    //while(q->count == 0){usleep(10);}
    //printf("dq: ");
    //printf(".");
    pthread_mutex_lock(&q->lock);
    while (q->count == 0) pthread_cond_wait(&q->cond, &q->lock);
    memcpy(out,&q->tasks[--q->count],size);
    //printf("DQ: %u\n",out->value);
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
    //usleep(1);
}

void* worker_thread(void* arg){
    shm_read* data = (shm_read*)arg;
    printf(".");
    for(;;){
        Task this;
        dequeue(&input,sizeof(Task),&this);
        //Task* this = (Task*)arg;

        u8 slot = this.slot;
        u32 val = this.value;
        u32 lim = sqrt(this.value);
        int c = 0;
        //printf("task: s%u, v%u, l%u, load:%lf\n",slot,val,lim,data->load[slot]);
        for(u32 i = 3; i <= lim; i += 2){
            //printf("pre ");
            //printf("post\n");
            //data->load[slot] = data->load[slot]+(100./SHIFTS)/(lim*.5);
            if(val%i==0){
                sem_wait(data->sem_pool[slot]);
                c++; 
                //printf("p");
                data->serverflag[slot] = SLOT_READY;
                data->slot[slot] = i;
                printf("Factor of %u <lim: %u>: [%u] -> %u ::: %lf\n",val,lim,slot,i,data->load[slot]);
                /*while(data->serverflag[slot] == SLOT_READY){
                    printf("send....");
                    pthread_cond_wait(&data->slotcond[slot],&data->slotlock[slot]);
                    printf("RECV\n");
                }*/
                sem_post(data->sem_pool[slot]);
                usleep(100);
            }//else printf("p");
        }

        sem_wait(data->sem);
        data->load[slot] += (1./SHIFTS);
        sem_post(data->sem);

        printf("finished task %u [%u, %i primes] + wrote to %u status slot\n",slot,this.value,c,32*slot+this.offset);
        status[32*slot+this.offset] = 1;

        /*pthread_mutex_lock(&data->slotlock[slot]);
        data->serverflag[slot] = SLOT_EMPTY;
        printf("finished task %u [%u]\n",slot,data->serverflag[slot]);
        pthread_mutex_unlock(&data->slotlock[slot]);*/

        //u32 out = 3456;
        //push(output,sizeof(u32),&out);
        //usleep(10);
    }
}

int main(int argc, char** argv){
    struct pollfd fds[1];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    int shm_fd;
    shm_read* data = shm_create(&shm_fd,1);

    setbuf(stdout, NULL);

    queue_list_init(&input);
    memset(status,0,SHIFTS*POOL);
    //queue_list_init(&output);
    printf("queues ok\n");


    // processing
    const int cc = 1;//POOL*SHIFTS;
    pthread_t workers[cc];
    for(int i = 0; i < cc; i++) pthread_create(&workers[i],NULL,worker_thread,(void*)&data);
    u8 count = 0;
    printf("workers ok\n");

    printf("LOCKING DOWN SERVER");
    sem_wait(data->sem);
    while(1);

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



        sem_wait(data->sem);
        //pthread_mutex_lock(&data->lock);

        /*printf("\033[s");
        printf("\033[5A");
        printf("\033[K");
        printf("clien f:%d s:%d c:%i",data->clientflag,data->clientslot,clock());
        printf("\033[u");*/

        //while(count >= POOL);
        while(data->clientflag == 1){
            //data->serverflag[count] = 1;
            u32 process = data->clientslot;
            printf("POP! %u\n",process);
            u8 slot = SLOT_BUSY;
            for(int i = 0; i < POOL; i++){
                sem_wait(data->sem_pool[i]);
                if(data->serverflag[i] == SLOT_EMPTY){
                    slot = i;
                    data->serverflag[i] = SLOT_WORKING;
                    break;
                }else printf("%ino\n",i);
                sem_post(data->sem_pool[i]);
            }
            if(slot == SLOT_BUSY) break;//printf("busy!\n");
            data->clientflag = 0;
            data->clientslot = 0;
            data->slot[slot] = 0;
            data->load[slot] = 0;
            memset(status+SHIFTS*slot*sizeof(u8),0,SHIFTS*sizeof(u8));
            //u8 slot = count++;
            /*if(data->serverflag[count] != SLOT_EMPTY){
                data->clientflag = SLOT_BUSY;
                continue;
            }else data->serverflag[count] = SLOT_WORKING;*/
            

            for(int i = 0; i < SHIFTS; i++){
                u32 shifted = (process>>i)|(process<<(SHIFTS-i));
                Task task;// = {input.count,shifted};
                task.slot = slot;
                task.offset = SHIFTS*slot+i;
                task.value = shifted;
                enqueue(&input,sizeof(Task),(void*)&task);
                //printf("TASK: s:%u v:%u\n",task.slot,task.value);
            }
            //count = (count+1)%POOL;
        }

        for(int i = 0; i < POOL; i++){
            u8 complete = 0;
            for(int j = 0; j < SHIFTS; j++){
                if(status[j] == 0) complete++;
            }
            if(complete == 0){
                data->serverflag[i] = SLOT_EMPTY;
                printf("slot %i finished!-------------------\n",i);
                memset(status+SHIFTS*i*sizeof(u8),0,SHIFTS*sizeof(u8));
            }/*else{
                if(complete < SHIFTS) printf("complete: %i\n",complete);
            }*/
        }

        //sem_post(&data->sem);
        //usleep(1);
        sem_post(data->sem);
    }

    for(int i = 0; i < POOL*SHIFTS; i++) pthread_cancel(workers[i]);

    queue_list_destroy(&input);
    //queue_list_destroy(&output);
    
    shm_destroy(&shm_fd,data,1);
    return 0;
}