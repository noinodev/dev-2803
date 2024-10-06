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
    void (*work)(shm_read*,struct Task*);
} Task;

// Circular queue
typedef struct {
    Task tasks[4*SHIFTS*POOL];
    /*int head;
    int tail;*/
    int count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} TaskQueue;

pthread_mutex_t lock;

TaskQueue input;
//u8 status[SHIFTS*POOL];
//double load[POOL];
TaskQueue output;
State state[POOL];
//double progress[POOL];

void queue_list_init(TaskQueue* q){
    pthread_mutex_init(&q->lock,NULL);
    pthread_cond_init(&q->cond,NULL);
    q->count = 0;
    //q->head = 0;
    //q->tail = 0;
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

void dequeue_nb(TaskQueue* q, size_t size, Task* out){
    //while(q->count == 0){usleep(10);}
    //printf("dq: ");
    //printf(".");
    pthread_mutex_lock(&q->lock);
    if(q->count == 0) goto end;
    memcpy(out,&q->tasks[--q->count],size);
    //printf("DQ: %u\n",out->value);
    pthread_cond_signal(&q->cond);

    end:
    pthread_mutex_unlock(&q->lock);
    //usleep(1);
}

void work_trialdivision(shm_read* data, Task* task){
    //printf("trialdiv\n");
    u8 slot = task->slot;
    u32 val = task->value;
    u32 lim = sqrt(task->value);
    for(u32 i = 2; i <= lim; i += 2){
        pthread_mutex_lock(&lock);
        state[slot].load += (1./SHIFTS)/(lim*.5);
        pthread_mutex_unlock(&lock);
        if(val%i==0){
            Task task;
            task.slot = slot;
            task.offset = 1;
            task.value = i;
            task.work = NULL;
            enqueue(&output,sizeof(Task),&task);
        }
        if(i==2)i++;
        usleep(1);
    }
}

void* worker_thread(void* arg){
    shm_read* data = (shm_read*)arg;
    //printf(".");
    for(;;){
        Task this;
        dequeue(&input,sizeof(Task),&this);

        //printf("working! ");

        if(this.work != NULL) this.work(data,&this);
        //Task* this = (Task*)arg;

        /*sem_wait(data->sem);
        data->load[slot] += (1./SHIFTS);
        sem_post(data->sem);*/

        //printf("finished task %u [%u, %i primes] + wrote to %u status slot\n",slot,this.value,c,32*slot+this.offset);
        //status[32*slot+this.offset] = 1;
        pthread_mutex_lock(&lock);
        state[this.slot].tasks++;
        pthread_mutex_unlock(&lock);

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
    queue_list_init(&output);
    //memset(status,0,SHIFTS*POOL);
    //memset(load,0,POOL*sizeof(double));
    memset(state,0,POOL*sizeof(State));
    //queue_list_init(&output);
    printf("queues ok\n");

    pthread_mutex_init(&lock,NULL);


    // processing
    const int cc = POOL*SHIFTS;
    pthread_t workers[cc];
    for(int i = 0; i < cc; i++) pthread_create(&workers[i],NULL,worker_thread,(void*)&data);
    u8 count = 0;
    printf("workers ok\n");

    //printf("LOCKING DOWN SERVER");
    //sem_wait(data->sem);
    //while(1);

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

        while(data->clientflag == 1){
            sem_wait(data->sem);
            u32 process = data->clientslot;
            printf("POP! %u\n",process);
            u8 slot = SLOT_BUSY;
            for(int i = 0; i < POOL; i++){
                if(data->serverflag[i] == SLOT_EMPTY){
                    slot = i;
                    data->serverflag[i] = SLOT_WORKING;
                    sem_post(data->sem);
                    break;
                }
            }
            data->clientflag = 0;
            data->clientslot = 0;
            if(slot == SLOT_BUSY){
                data->clientflag = SLOT_BUSY;
                sem_post(data->sem);
                printf("the server is busy\n");
                break;//printf("busy!\n");
            }
            data->slot[slot] = 0;
            //data->load[slot] = 0;
            sem_post(data->sem);

            state[slot].load = 0;
            state[slot].tasks = 0;
            state[slot].time = clock();

            for(int i = 0; i < SHIFTS; i++){
                u32 shifted = (process>>i)|(process<<(SHIFTS-i));
                Task task;
                task.slot = slot;
                task.offset = SHIFTS*slot+i;
                task.value = shifted;
                task.work = work_trialdivision;
                enqueue(&input,sizeof(Task),(void*)&task);
            }
        }

        sem_wait(data->sem);
        for(int i = 0; i < POOL; i++){
            data->state[i].load = state[i].load;
            data->state[i].time = clock()-state[i].time;
            if(data->state[i].tasks == SHIFTS){
                state[i].tasks = 0;
                state[i].time = 0;
                state[i].load = 0;
                data->serverflag[i] = SLOT_EMPTY;
            }else print_bars(data->state,10,POOL);
        }
        sem_post(data->sem);

        Task this = {0,0,0};
        dequeue_nb(&output,sizeof(Task),&this);

        if(this.offset == 1){
            pthread_mutex_lock(&lock);
            sem_wait(data->sem);
            data->serverflag[this.slot] = SLOT_READY;
            data->slot[this.slot] = this.value;
            sem_post(data->sem);

            //printf("DEQUEUED AND WAITING <factor %u to slot %u>\n",this.value,this.slot);

            int wait = 2;
            while(data->serverflag[this.slot] == SLOT_READY){
                usleep(wait);
                wait *= 2;
                if(wait > 8192){
                    printf("client no response %i\n",wait);
                    break;
                }
            }
            sem_wait(data->sem);
            data->serverflag[this.slot] = SLOT_WORKING;
            data->slot[this.slot] = 0;
            sem_post(data->sem);
            pthread_mutex_unlock(&lock);
        }

        //sem_post(&data->sem);
        usleep(10);
        //sem_post(data->sem);
    }

    for(int i = 0; i < POOL*SHIFTS; i++) pthread_cancel(workers[i]);
    //////////////////pthread_cancel(srv);

    queue_list_destroy(&input);
    queue_list_destroy(&output);
    
    shm_destroy(&shm_fd,data,1);
    return 0;
}