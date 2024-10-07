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

#define QUEUE_SIZE 8192

// Task struct for queue
struct Task;
typedef struct Task {
    u8 slot;
    u32 value;
    void (*work)(shm_read*,struct Task*);
} Task;

// Circular queue
typedef struct {
    Task tasks[QUEUE_SIZE];
    int count,head,tail;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} TaskQueue;

pthread_mutex_t lock;
TaskQueue input;
TaskQueue output;
State state[POOL];

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

int enqueue(TaskQueue* q, size_t size, Task* i){
    //while(q->count >= POOL){usleep(100);}
    //while (q->count >= sizeof(q->tasks) / sizeof(Task)) pthread_cond_wait(&q->cond, &q->lock);
    if(q->count >= QUEUE_SIZE){
        printf("NQ FAIL: %i\n",q->count);
        return -1;
    }
    pthread_mutex_lock(&q->lock);
    //printf("cpy...");
    memcpy(&q->tasks[q->tail],i,size);
    q->tail = (q->tail+1)%QUEUE_SIZE;
    q->count++;
    //printf("signal...");
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
    return 0;
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
    memcpy(out,&q->tasks[q->head],size);
    q->head = (q->head+1)%QUEUE_SIZE;
    q->count--;
    //printf("DQ: %u\n",out->value);
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
    //usleep(1);
}

int dequeue_nb(TaskQueue* q, size_t size, Task* out){
    //while(q->count == 0){usleep(10);}
    //printf("dq: ");
    //printf(".");
    if(q->count == 0) return -1;
    pthread_mutex_lock(&q->lock);
    memcpy(out,&q->tasks[q->head],size);
    q->head = (q->head+1)%QUEUE_SIZE;
    q->count--;
    //printf("DQ: %u\n",out->value);
    pthread_cond_signal(&q->cond);

    pthread_mutex_unlock(&q->lock);
    return 0;
    //usleep(1);
}

void work_trialdivision(shm_read* data, Task* task){
    //printf("trialdiv\n");
    u8 slot = task->slot;
    u32 val = task->value;
    int lim = sqrt(val);
    u32 i = 2;
    //printf("val:%i\n",val);
    double total_space = 1./SHIFTS;
    double local_space = (1./SHIFTS)/(lim*.5);
    int c = 0;
    
    //printf("val: %i\n",val);
    for(int i = 2; i <= lim; i += 2){
        c++;
        //printf("%u\n",i);
        pthread_mutex_lock(&lock);
        state[slot].load += local_space;
        pthread_mutex_unlock(&lock);
        if(val%i==0){
            Task task;
            task.slot = slot;
            task.value = i;
            task.work = NULL;
            enqueue(&output,sizeof(Task),&task);
            //printf("Factor found: %u\n",i);
        }
        if(i == 2) i++;
        if(c%100 == 0)usleep(1);
    }
}

void work_test(shm_read* data, Task* task){
    u8 slot = task->slot;
    u32 val = task->value;

    //for(int i = 0; i < 10; i++){
    usleep((10+rand()%90)*1000);
    pthread_mutex_lock(&lock);
    Task that;
    that.value = val;
    that.slot = slot;
    that.work = NULL;
    enqueue(&output,sizeof(Task),&that);
    pthread_mutex_unlock(&lock);
    //}
}

void work_finish(shm_read* data, Task* task){
    u8 slot = task->slot;

    printf("slot %i finished in %i seconds\n",slot,data->state[slot].time);
    data->serverflag[slot] = SLOT_FINISHED;
    data->state[slot].load = state[slot].load; 
    data->state[slot].time = time(NULL)-state[slot].time;
    data->state[slot].tasks = state[slot].tasks;
    sem_post(data->sem);

    int wait = 1;
    while(data->serverflag[slot] == SLOT_FINISHED){
        usleep(wait);
        wait *= 2;
        if(wait > 8192){
            printf("client no ack finish %i\n",wait);
            break;
        }
    }
    sem_wait(data->sem);
    data->serverflag[slot] == SLOT_EMPTY;
    state[slot].tasks = 0;
    state[slot].time = 0;
    state[slot].load = 0;
}

void* worker_thread(void* arg){
    shm_read* data = (shm_read*)arg;
    //printf(".");
    for(;;){
        Task this;
        dequeue(&input,sizeof(Task),&this);

        if(this.work != NULL) this.work(data,&this);

        pthread_mutex_lock(&lock);
        state[this.slot].tasks++;
        if(state[this.slot].tasks >= state[this.slot].tasks_total){
            //printf("finisher task\n");
            Task that;
            that.slot = this.slot;
            that.value = 0;
            that.work = work_finish;
            enqueue(&output,sizeof(Task),&that);
        }   
        //printf("tasks: %u/%i\n",state[this.slot].tasks,SHIFTS);
        pthread_mutex_unlock(&lock);
    }
}

u8 slot_get(shm_read* data){
    u8 slot = SLOT_BUSY;
    for(int i = 0; i < POOL; i++){
        if(data->serverflag[i] == SLOT_EMPTY){
            slot = i;
            data->serverflag[i] = SLOT_WORKING;
            return slot;
        }
    }
    //if(slot == SLOT_BUSY){
        //data->clientflag = SLOT_BUSY;
        //sem_post(data->sem);
    printf("the server is busy\n");
    return slot;//printf("busy!\n");
    //}
    //data->slot[slot] = 0;
    //return slot;
}

int main(int argc, char** argv){
    struct pollfd fds[1];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    int shm_fd;
    shm_read* data = shm_create(&shm_fd,1);

    setbuf(stdout, NULL);
    printf("\033[2J");

    queue_list_init(&input);
    queue_list_init(&output);

    memset(state,0,POOL*sizeof(State));
    
    printf("queues ok\n");

    pthread_mutex_init(&lock,NULL);


    // processing
    const int cc = POOL*SHIFTS;
    pthread_t workers[cc];
    for(int i = 0; i < cc; i++) pthread_create(&workers[i],NULL,worker_thread,(void*)&data);
    u8 count = 0;
    printf("workers ok\n");

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
        if(data->clientflag == 1){

            u32 process = data->clientslot;
            if(process == 0){
                u8 can_test = 1;
                for(int i = 0; i < POOL; i++){
                    if(data->serverflag[i] != SLOT_EMPTY){
                        can_test = 0;
                        break;
                    }
                }
                if(can_test == 1){
                    printf("--TESTING MODE--\n");
                    for(int i = 0; i < 3; i++){
                        u8 slot = slot_get(data);
                        state[slot].load = 0;
                        state[slot].tasks = 0;
                        state[slot].time = time(NULL);
                        state[slot].tasks_total = 10;
                        for(int j = 0; j < 10; j++){
                            u32 value = 10*i+j;
                            Task task;
                            task.slot = slot;
                            task.value = 10*i+j;
                            task.work = work_test;
                            enqueue(&input,sizeof(Task),&task);
                        }
                    }
                }
            }else{
                u8 slot = slot_get(data);
                printf("POP! %u to slot: %u\n",process,slot);

                if(slot != SLOT_BUSY){
                    state[slot].load = 0;
                    state[slot].tasks = 0;
                    state[slot].time = time(NULL);
                    state[slot].tasks_total = SHIFTS;

                    for(int i = 0; i < SHIFTS; i++){
                        u32 value = (process>>i)|(process<<(SHIFTS-i));
                        Task task;
                        task.slot = slot;
                        task.value = value;
                        task.work = work_trialdivision;
                        int ret = enqueue(&input,sizeof(Task),(void*)&task);
                    }
                }
            }
            data->clientflag = 0;
            data->clientslot = 0;
        }

        //sem_wait(data->sem);
        // synchronizer loop because shmem isnt working the way i need it to
        print_bars(state,10,POOL);
        for(int i = 0; i < POOL; i++){
            data->state[i].load = state[i].load;
            data->state[i].time = time(NULL)-state[i].time;
            data->state[i].tasks = state[i].tasks;
            data->state[i].tasks_total = state[i].tasks_total;
            /*if(state[i].tasks >= SHIFTS){
                printf("slot %i finished in %i seconds\n",i,data->state[i].time);
                data->serverflag[i] = SLOT_FINISHED;
                data->state[i].load = state[i].load; 
                data->state[i].time = time(NULL)-state[i].time;
                data->state[i].tasks = state[i].tasks;
                sem_post(data->sem);

                int wait = 1;
                while(data->serverflag[i] == SLOT_FINISHED){
                    usleep(wait);
                    wait *= 2;
                    if(wait > 8192){
                        printf("client no ack finish %i\n",wait);
                        break;
                    }
                }
                sem_wait(data->sem);
                data->serverflag[i] == SLOT_EMPTY;
                state[i].tasks = 0;
                state[i].time = 0;
                state[i].load = 0;
            }*/
        }
        sem_post(data->sem);

        Task this = {0,0,NULL};
        int dq = dequeue_nb(&output,sizeof(Task),&this);
        if(dq != -1){
            //while(data->serverflag[this.slot] == SLOT_FINISHED);
            if(this.work != NULL){
                //printf("dq finisher\n");
                this.work(data,&this);
            }else{
                pthread_mutex_lock(&lock);
                sem_wait(data->sem);
                u8 last = data->serverflag[this.slot];
                data->serverflag[this.slot] = SLOT_READY;
                data->slot[this.slot] = this.value;
                sem_post(data->sem);
                

                int wait = 1;
                while(data->serverflag[this.slot] == SLOT_READY){
                    usleep(wait);
                    wait *= 2;
                    if(wait > 8192){
                        printf("client no response %i\n",wait);
                        break;
                    }
                }
                sem_wait(data->sem);
                data->serverflag[this.slot] = last;
            }
            //else data->serverflag[this.slot] = SLOT_FINISHED;
                /*printf("slot %i finished in %i seconds\n",this.slot,data->state[this.slot].time);
                data->serverflag[this.slot] = SLOT_FINISHED;
                data->state[this.slot].load = state[this.slot].load; 
                data->state[this.slot].time = time(NULL)-state[this.slot].time;
                data->state[this.slot].tasks = state[this.slot].tasks;
                sem_post(data->sem);

                while(data->serverflag[this.slot] == SLOT_FINISHED){
                    usleep(wait);
                    wait *= 2;
                    if(wait > 8192){
                        printf("client no response %i\n",wait);
                        break;
                    }
                
                sem_wait(data->sem);
                state[this.slot].tasks = 0;
                state[this.slot].time = 0;
                state[this.slot].load = 0;
            }*/
            data->slot[this.slot] = 0;
            sem_post(data->sem);
            pthread_mutex_unlock(&lock);
        }

        usleep(1);
    }

    for(int i = 0; i < POOL*SHIFTS; i++) pthread_cancel(workers[i]);

    queue_list_destroy(&input);
    queue_list_destroy(&output);
    
    shm_destroy(&shm_fd,data,1);
    return 0;
}