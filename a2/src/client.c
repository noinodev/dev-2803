#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <pthread.h>
#include <sys/sem.h>
#include "shmem.h"

//double progress[POOL];
double load[POOL];
int bar_count;
pthread_mutex_t lock;

void page_main(shm_read* data){
    /*printf("\033[1A");
    printf("\033[K");
    printf("clien f:%d s:%d c:%i",data->clientflag,data->clientslot,clock());*/
}

void page_test(shm_read* data){
    /*printf("\033[1A");
    printf("\033[K");
    printf("time: %i",-clock());*/
}

u8 push(shm_read* data,char* buffer){
    pthread_mutex_lock(&data->lock);
    char* endptr;
    u32 i = strtoul(buffer,&endptr,10);
    if(endptr == NULL || (*endptr != '\0' && *endptr != '\n')) return 0;
    printf("PUSH! %i %d\n",i,*endptr);
    data->clientslot = i;
    data->clientflag = 1;
    pthread_mutex_unlock(&data->lock);
    return 1;
}

void* listener(void* arg){
    shm_read* data = (shm_read*)arg;
    for(;;){

        bar_count = 0;
        memset(load,0,POOL*sizeof(double));
        //printf("a");
        pthread_mutex_lock(&lock);
        for(int i = 0; i < POOL; i++){
            //pthread_mutex_lock(&data->slotlock[i]);
            //page(data);
            //printf("b");
            if(data->serverflag[i] != SLOT_EMPTY){
                //printf("c[%u,%lf]",data->serverflag[i],data->load[i]);
                load[bar_count] = data->load[i];
                bar_count++;
                //printf("Factor found: [%i] -> %u\n",i,data->slot[i]);
                if(data->serverflag[i] == SLOT_READY){
                    printf("Factor found: [%i] -> %u\n",i,data->slot[i]);
                    data->serverflag[i] = SLOT_WORKING;
                    pthread_cond_signal(&data->slotcond[i]);
                }
            }

            //pthread_mutex_unlock(&data->slotlock[i]);
        }
        pthread_mutex_unlock(&lock);
        usleep(100);
    }


    return NULL;
}

int main(int argc, char** argv){
    // feature set:
    /*
        poll integer input with poll+fgets for stdin
        mutex lock 
    */

    setbuf(stdout, NULL);

    struct pollfd fds[1];
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    int shm_fd;
    shm_read* data = shm_create(&shm_fd,0);
    void (*page)(shm_read*) = page_main;


    printf("CLIENT STARTING\n");
    pthread_mutex_lock(&data->lock);
    printf("IF MUTEX IS WORKING THIS SHOULD NOT PRINT\n");

    pthread_t listener_thread;
    pthread_create(&listener_thread,NULL,listener,(void*)data);

    pthread_mutex_init(&lock,NULL);

    u32 cache[POOL*32];
    memset(cache,0,POOL*32*sizeof(u32));
    bar_count = 0;

    printf("Enter an integer pls\n");

    for(;;){
        if(page != NULL && clock() % 10 == 0 ){
            printf("\033[s");
            page(data);
            printf("\033[u");
        }

        int ret = poll(fds,1,0);
        if(ret > 0 && fds[0].revents & POLLIN){
            // receive input
            u8 buffer_input[INPUT];
            fgets(buffer_input,sizeof(buffer_input), stdin);

            //printf("in: %s",buffer_input);

            if(strncmp(buffer_input,"quit",4) == 0) break;
            //else if(strncmp(buffer_input,"help",4) == 0) page = page_test;
            /*else if(strlen(buffer_input) == 0) page = page_main;*/
            else if(strncmp(buffer_input,"0",1) == 0) page = page_test;// test mode
            else push(data,buffer_input);


            // write input to shmem
        }

        pthread_mutex_lock(&lock);
        int bar_width = 64;
        if(bar_count > 0){
            printf("\033[s");
            printf("\033[8A");
            printf("\033[K");
            printf("\n");
            printf("\033[K");
            printf("\r");
            for(int i = 0; i < bar_count; i++){
                if(load[i] > 0){
                double d = (int)(load[i]*1000)/1000.;
                    printf("<%lf>",d);
                    printf("[");
                    for(int j = 0; j < bar_width/bar_count; j++){
                        char k = '.';
                        if(j < d*(bar_width/bar_count)) k = '|';
                        printf("%c",k);
                    }
                    printf("] , ");
                }
            }
            printf("\033[u");
        }
        pthread_mutex_unlock(&lock);
        usleep(10);
    }

    shm_destroy(&shm_fd,data,0);

    return 0;
}