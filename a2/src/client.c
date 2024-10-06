#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <pthread.h>
#include <sys/sem.h>
#include <semaphore.h>
#include "shmem.h"

//double progress[POOL];
//double load[POOL];
int bar_count;
pthread_mutex_t lock;

u8 push(shm_read* data,char* buffer){
    sem_wait(data->sem);
    char* endptr;
    u32 i = strtoul(buffer,&endptr,10);
    if(endptr == NULL || (*endptr != '\0' && *endptr != '\n')) return 0;
    printf("PUSH! %i %d\n",i,*endptr);
    data->clientslot = i;
    data->clientflag = 1;
    sem_post(data->sem);
    return 1;
}

int main(int argc, char** argv){
    // feature set:
    /*
        poll integer input with poll+fgets for stdin
        mutex lock 
    */
    State state[POOL] = {0,0,0};

    setbuf(stdout, NULL);

    struct pollfd fds[1];
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    int shm_fd;
    shm_read* data = shm_create(&shm_fd,0);

    pthread_mutex_init(&lock,NULL);

    //u32 cache[POOL*32];
    //memset(cache,0,POOL*32*sizeof(u32));
    bar_count = 0;

    printf("Enter an integer pls\n");

    for(;;){
        int ret = poll(fds,1,0);
        if(ret > 0 && fds[0].revents & POLLIN){
            // receive input
            u8 buffer_input[INPUT];
            fgets(buffer_input,sizeof(buffer_input), stdin);

            //printf("in: %s",buffer_input);

            if(strncmp(buffer_input,"quit",4) == 0) break;
            //else if(strncmp(buffer_input,"help",4) == 0) page = page_test;
            /*else if(strlen(buffer_input) == 0) page = page_main;*/
            //else if(strncmp(buffer_input,"0",1) == 0) page = page_test;// test mode
            else push(data,buffer_input);


            // write input to shmem
        }

        if(data->clientflag == SLOT_BUSY){
            printf("the server is busy\n");
            sem_wait(data->sem);
            data->clientflag = 0;
            sem_post(data->sem);
        }

        bar_count = 0;
        pthread_mutex_lock(&lock);
        //memset(load,0,POOL*sizeof(double));
        //printf("a");
        //sem_wait(data->sem);
        for(int i = 0; i < POOL; i++){
            //pthread_mutex_lock(&data->slotlock[i]);
            //page(data);
            //printf("b");

            u8 flag = data->serverflag[i];
            if(flag != SLOT_EMPTY){
                //printf("WORKING %u\n",flag);
                state[i].load = data->state[i].load;
                state[i].time = data->state[i].time;
                if(state[i].load > 0) bar_count++;
                //printf("Factor found: [%i] -> %u\n",i,data->slot[i]);
                if(flag == SLOT_READY){
                    //sem_wait(data->sem_pool[i]);
                    sem_wait(data->sem);
                    printf("Factor found: [%i] -> %u\n",i,data->slot[i]);
                    data->serverflag[i] = SLOT_WORKING;
                    sem_post(data->sem);
                    //sem_post(data->sem_pool[i]);
                    //pthread_cond_signal(&data->slotcond[i]);
                }
                //sem_post(data->sem);
            }

            //pthread_mutex_unlock(&data->slotlock[i]);
        }
        //sem_post(data->sem);
        pthread_mutex_unlock(&lock);

        //pthread_mutex_lock(&lock);
        int bar_width = 64;
        if(bar_count > 0) print_bars(state,bar_width/(bar_count),POOL);
        //pthread_mutex_unlock(&lock);
        usleep(10);
    }

    shm_destroy(&shm_fd,data,0);

    return 0;
}