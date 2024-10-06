#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <pthread.h>
#include "shmem.h"

void page_main(shm_read* data){
    printf("\033[5A");
    printf("\033[K");
    printf("clien f:%d s:%d c:%i",data->clientflag,data->clientslot,clock());
}

void page_test(shm_read* data){
    printf("\033[1A");
    printf("\033[K");
    printf("time: %i",-clock());
}

u8 push(shm_read* data,char* buffer){
    char* endptr;
    u32 i = strtol(buffer,&endptr,10);
    if(endptr == NULL || (*endptr != '\0' && *endptr != '\n')) return 0;
    printf("PUSH! %i %d\n",i,*endptr);
    data->clientslot = i;
    data->clientflag = 1;
    return 1;
}   

u32 pop(shm_read* data){
    return 0;
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
    char* shm;
    shm_read* data = (shm_read*)shm_create(&shm_fd,shm,0);
    void (*page)(shm_read*) = page_main;

    printf("Enter an integer pls\n");

    for(;;){
        pthread_mutex_lock(&data->lock);
        //
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

            printf("in: %s",buffer_input);

            if(strncmp(buffer_input,"quit",4) == 0) break;
            //else if(strncmp(buffer_input,"help",4) == 0) page = page_test;
            /*else if(strlen(buffer_input) == 0) page = page_main;
            else if(strncmp(buffer_input,"0",1) == 0) page = page_test;// test mode*/
            else push(data,buffer_input);


            // write input to shmem
        }
        pthread_mutex_unlock(&data->lock);
        usleep(100);
    }

    shm_destroy(&shm_fd,shm,0);

    return 0;
}