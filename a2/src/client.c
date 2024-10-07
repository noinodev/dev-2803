#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <pthread.h>
#include <semaphore.h>
#include "shmem.h"

// global local mutex
pthread_mutex_t lock;

// push stdin input to server
void push(shm_read* data,char* buffer){
    sem_wait(data->sem);
    char* endptr;
    u32 i = strtoul(buffer,&endptr,10);
    // make sure input is integer
    if(endptr == NULL || (*endptr != '\0' && *endptr != '\n')){
        printf("input must be an unsigned integer\n");
        goto end;
    }

    // count busy slots on server
    int c = 0;
    for(int i = 0; i < POOL; i++){
        if(data->serverflag[i] != SLOT_EMPTY) c++;
    }

    if(i == 0){
        // test mode cant have any busy slots
        if(c > 0){
            printf("cannot enter testing mode while server is processing\n");
            goto end;
        }else printf("--TESTING MODE--\n");
    }else{
        // trial division mode needs at least 1 empty slot
        if(c >= POOL){
            printf("server is busy\n");
            goto end;
        }
        // no negative
        if(i < 0){
            printf("input cannot be negative\n");
            goto end;
        }
        
        printf("%i enqueued\n",i);
    }
    data->clientslot = i;
    data->clientflag = 1;
    // i would use return but i dont want to hang my semaphore
    // i really like goto actually, change my mind? flow this flow that... but erm its awesome? ever considered that?
    end:
    sem_post(data->sem);
}

int main(int argc, char** argv){
    // initialize global state, can do it here unlike the server because single thread and push doesnt need it
    State state[POOL] = {0,0,0};
    setbuf(stdout, NULL); // disable stout buffering
    printf("\033[2J"); // clear terminal

    // poll init
    struct pollfd fds[1];
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    // shm init
    int shm_fd;
    shm_read* data = shm_create(&shm_fd,0);

    // mutex init
    pthread_mutex_init(&lock,NULL);

    // Event loop
    printf("Enter an integer.\n");
    for(;;){
        // poll input
        int ret = poll(fds,1,0);
        if(ret > 0 && fds[0].revents & POLLIN){
            // receive input if poll 
            u8 buffer_input[INPUT];
            fgets(buffer_input,sizeof(buffer_input), stdin);

            // decide what to do
            if(strncmp(buffer_input,"quit",4) == 0) break;
            else push(data,buffer_input);
        }

        // server told client that its busy, kind of a redundancy but confirms server side
        if(data->clientflag == SLOT_BUSY){
            printf("the server is busy\n");
            sem_wait(data->sem);
            data->clientflag = 0;
            sem_post(data->sem);
        }

        // slot state sync
        pthread_mutex_lock(&lock);
        for(int i = 0; i < POOL; i++){
            // get flag for slot
            u8 flag = data->serverflag[i];
            if(flag != SLOT_EMPTY){
                // if not empty, pull shared memory data to local state. 
                state[i].load = data->state[i].load;
                state[i].time = data->state[i].time;
                state[i].tasks = data->state[i].tasks;

                // receive a factor from server and set slot back
                if(flag == SLOT_READY && data->slot[i] > 0){
                    sem_wait(data->sem);
                    printf("Factor found: [%i] -> %u\n",i,data->slot[i]);
                    data->slot[i] = 0;
                    data->serverflag[i] = SLOT_WORKING;
                    sem_post(data->sem);
                }
                // receive a finish signal from server and reset local state
                if(flag == SLOT_FINISHED){
                    printf("Finished processing for slot %u in %i seconds\n",i,state[i].time);
                    data->serverflag[i] = SLOT_EMPTY;
                    state[i].load = 0;
                    state[i].time = 0;
                    state[i].tasks = 0;
                }
            }
        }
        pthread_mutex_unlock(&lock);

        print_bars(state,64,POOL); // this needs local state because the shared memory is volatile
        usleep(10); // just makes the print bars flicker less i suppose, might help sync too
    }

    shm_destroy(&shm_fd,data,0);

    return 0;
}