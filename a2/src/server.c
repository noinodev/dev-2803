#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <poll.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "shmem.h"

#define QUEUE_SIZE 8192

// task struct for queue
// function pointer for work was fun
struct Task;
typedef struct Task {
    u8 slot;
    u32 value;
    void (*work)(shm_read*,struct Task*);
} Task;

// circular buffer
typedef struct {
    Task tasks[QUEUE_SIZE];
    int count,head,tail;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} TaskQueue;

// global variables
pthread_mutex_t lock;
TaskQueue input;
TaskQueue output;
State state[POOL];

// queue init/destroy functions
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

// queue push and pop functions
// non blocking enqueue
int enqueue(TaskQueue* q, size_t size, Task* i){
    if(q->count >= QUEUE_SIZE) return -1;

    // lock and copy arg to buffer
    pthread_mutex_lock(&q->lock);
    memcpy(&q->tasks[q->tail],i,size);
    q->tail = (q->tail+1)%QUEUE_SIZE;
    q->count++;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

// blocking dequeue, for dequeueing on worker threads
void dequeue(TaskQueue* q, size_t size, Task* out){
    // lock and copy buffer to arg
    pthread_mutex_lock(&q->lock);
    // wait for enqueue signal
    while (q->count == 0) pthread_cond_wait(&q->cond, &q->lock);
    memcpy(out,&q->tasks[q->head],size);
    q->head = (q->head+1)%QUEUE_SIZE;
    q->count--;
    pthread_cond_signal(&q->cond); // not really needed, enqueue used to be blocking but i changed it
    pthread_mutex_unlock(&q->lock);
}

// non blocking dequeue, for dequeueing on the main thread
int dequeue_nb(TaskQueue* q, size_t size, Task* out){
    if(q->count == 0) return -1;

    // lock and copy buffer to arg
    pthread_mutex_lock(&q->lock);
    memcpy(out,&q->tasks[q->head],size);
    q->head = (q->head+1)%QUEUE_SIZE;
    q->count--;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
    return 0;
}


// task work functions for worker thread
void work_trialdivision(shm_read* data, Task* task){
    u8 slot = task->slot;
    u32 val = task->value;
    int lim = sqrt(val);
    u32 i = 2;

    // fractional value of each iteration in the loop, respecting load
    double local_space = (1./SHIFTS)/(lim*.5);
    int c = 0;
    
    for(int i = 2; i <= lim; i += 2){
        c++;
        //printf("%u\n",i);
        pthread_mutex_lock(&lock);
        state[slot].load += local_space;
        pthread_mutex_unlock(&lock);
        if(val%i==0){
            // found factor
            // push factor to output queue
            Task task;
            task.slot = slot;
            task.value = i;
            task.work = NULL;
            enqueue(&output,sizeof(Task),&task);
        }
        if(i == 2) i--;
        if(c%40 == 0)usleep(1); // no if made it too slow, no sleep made it too fast. good medium
    }
}

// test mode work function
void work_test(shm_read* data, Task* task){
    u8 slot = task->slot;
    u32 val = task->value;

    // wait a bit of time, lock and push value to output queue 
    usleep((10+rand()%90)*1000);
    pthread_mutex_lock(&lock);
    Task that;
    that.value = val;
    that.slot = slot;
    that.work = NULL;
    enqueue(&output,sizeof(Task),&that);
    pthread_mutex_unlock(&lock);
}

// task to indicate the sequence is finished and the slot should become available
void work_finish(shm_read* data, Task* task){
    u8 slot = task->slot;

    // synchronize local state with shmem for client
    printf("slot %i finished in %i seconds\n",slot,data->state[slot].time);
    data->serverflag[slot] = SLOT_FINISHED;
    data->state[slot].load = state[slot].load; 
    data->state[slot].time = time(NULL)-state[slot].time;
    data->state[slot].tasks = state[slot].tasks;
    sem_post(data->sem); // when the function is called the thread should be holding sem

    // wait for client to respond, about 10ms
    int wait = 1;
    while(data->serverflag[slot] == SLOT_FINISHED){
        usleep(wait);
        wait *= 2;
        if(wait > 8192){
            printf("client no ack finish %i\n",wait);
            break;
        }
    }

    // reset shared slot and local state for slot
    sem_wait(data->sem);
    data->serverflag[slot] == SLOT_EMPTY;
    state[slot].tasks = 0;
    state[slot].time = 0;
    state[slot].load = 0;
}

// worker thread function, dequeue task and operate
void* worker_thread(void* arg){
    shm_read* data = (shm_read*)arg;
    for(;;){
        // wait for task to become available
        Task this;
        dequeue(&input,sizeof(Task),&this);

        // execute task function pointer
        if(this.work != NULL) this.work(data,&this);

        // increment task counter for slot, eg. trial division has 32 tasks
        pthread_mutex_lock(&lock);
        state[this.slot].tasks++;

        // if all tasks are finished, push a work_finish task to output queue so edt can synchronize
        if(state[this.slot].tasks >= state[this.slot].tasks_total){
            Task that;
            that.slot = this.slot;
            that.value = 0;
            that.work = work_finish;
            enqueue(&output,sizeof(Task),&that);
        }   
        pthread_mutex_unlock(&lock);
    }
}

// get next available slot in shared memory and set it to working
u8 slot_get(shm_read* data){
    u8 slot = SLOT_BUSY;
    for(int i = 0; i < POOL; i++){
        if(data->serverflag[i] == SLOT_EMPTY){
            slot = i;
            data->serverflag[i] = SLOT_WORKING;
            return slot;
        }
    }
    printf("the server is busy\n");
    return slot;
}

int main(int argc, char** argv){
    // initialize poll
    struct pollfd fds[1];
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    // initialize shm and sem
    int shm_fd;
    shm_read* data = shm_create(&shm_fd,1);

    // set stdout to non-buffering
    setbuf(stdout, NULL);
    printf("\033[2J"); // clear terminal (ascii escape mi amor)

    queue_list_init(&input);
    queue_list_init(&output); // initialize global i/o queues

    memset(state,0,POOL*sizeof(State)); // initialize local global state
    
    printf("queues ok\n"); // debug

    pthread_mutex_init(&lock,NULL); // init server mutex


    // initialize thread pool
    const int cc = POOL*SHIFTS;
    pthread_t workers[cc];
    for(int i = 0; i < cc; i++) pthread_create(&workers[i],NULL,worker_thread,(void*)&data);
    u8 count = 0;
    printf("workers ok\n");

    // begin event loop
    for(;;){
        // supposedly shmem isnt released at program close so i have a graceful exit on server just in case
        // poll input for quit
        int ret = poll(fds,1,0);
        if(ret > 0 && fds[0].revents & POLLIN){
            // receive input
            u8 buffer_input[INPUT];
            fgets(buffer_input,sizeof(buffer_input), stdin);
            if(strncmp(buffer_input,"quit",4) == 0) break;
        }

        // check if client has sent something
        sem_wait(data->sem);
        if(data->clientflag == 1){

            u32 process = data->clientslot;
            if(process == 0){
                // user trying test mode
                // make sure the server isnt doing anything before going into test mode
                u8 can_test = 1;
                for(int i = 0; i < POOL; i++){
                    if(data->serverflag[i] != SLOT_EMPTY){
                        can_test = 0;
                        break;
                    }
                }
                if(can_test == 1){
                    // run 3 slots with 10 tasks each with corresponding numbers to push
                    // work_test function pointer
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
                // push 32 tasks to whatever slot it can get
                // work_trialdivision function pointer
                u8 slot = slot_get(data);
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

        // synchronizer loop because shmem isnt working the way i need it to
        // print loading bars and copy local state to shared memory start
        print_bars(state,10,POOL);
        for(int i = 0; i < POOL; i++){
            data->state[i].load = state[i].load;
            data->state[i].time = time(NULL)-state[i].time;
            data->state[i].tasks = state[i].tasks;
            data->state[i].tasks_total = state[i].tasks_total;
        }
        sem_post(data->sem);

        // dequeue output queue, for dispatching results to client. shmem only works properly on this thread so i just had to do it this way
        Task this = {0,0,NULL};
        int dq = dequeue_nb(&output,sizeof(Task),&this);
        if(dq != -1){
            if(this.work != NULL){
                // if an output task has a function pointer under work, then it is a finisher task
                // it will signal client and server to reset this slot
                this.work(data,&this);
            }else{
                // otherwise, push the factor in the task queue to the client
                // set slot flag to ready
                pthread_mutex_lock(&lock);
                sem_wait(data->sem);
                u8 last = data->serverflag[this.slot];
                data->serverflag[this.slot] = SLOT_READY;
                data->slot[this.slot] = this.value;
                printf("Factor found: [%i] -> %u\n",this.slot,this.value);
                sem_post(data->sem);

                // wait for client response about 10ms
                int wait = 1;
                while(data->serverflag[this.slot] == SLOT_READY){
                    usleep(wait);
                    wait *= 2;
                    if(wait > 8192){
                        printf("client no response %i\n",wait);
                        break;
                    }
                }
                // then set it back
                sem_wait(data->sem);
                data->serverflag[this.slot] = last;
                sem_post(data->sem);
            }
            data->slot[this.slot] = 0;
            pthread_mutex_unlock(&lock);
        }

        usleep(1);
    }

    // cleanup
    for(int i = 0; i < POOL*SHIFTS; i++) pthread_cancel(workers[i]);
    queue_list_destroy(&input);
    queue_list_destroy(&output);
    shm_destroy(&shm_fd,data,1);
    return 0;
}