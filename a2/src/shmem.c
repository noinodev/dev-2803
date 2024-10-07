#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <semaphore.h>
#include "shmem.h"

const char name[2][16] = {"/SHMEM","/SEM_MAIN"};

// open and link a named shared memory space
// if owner==1, also create and map the shared memory to the struct
shm_read* shm_create(int* shm_fd, u8 owner){
    // open shared memory
    *shm_fd = shm_open(name[0],O_CREAT|O_RDWR,0666);
    if (*shm_fd < 0) {
        perror("shm_open");
        return NULL;
    }else printf("shm ok\n");

    // truncate shared memory to size of shm_read
    if(owner){
        if(ftruncate(*shm_fd,sizeof(shm_read)) == -1){
            perror("ftruncate");
            close(*shm_fd);
            return NULL;
        }else printf("truncate ok\n");
    }

    // map shared memory
    shm_read* data = (shm_read*)mmap(0,sizeof(shm_read),PROT_READ|PROT_WRITE,MAP_SHARED,*shm_fd,0);
    if (data == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }else if(data == NULL){
        printf("no shm\n");
        return NULL;
    }else printf("map ok\n");

    // reset shared memory, in case it was left hanging after a server crash
    if(owner) memset(data,0,sizeof(shm_read));

    // this used to be an awesome mutex/cond with attributes thing before cygwin killed me
    // open named semaphore
    sem_t* sem = sem_open(name[1], O_CREAT|O_EXCL,0666,1);
    if(sem == SEM_FAILED)perror("semaphore:");
    else printf("semaphore ok\n");
    data->sem = sem;

    return data;
}

// unmap and unlink shm/sem
void shm_destroy(int* shm_fd, shm_read* data, u8 owner){
    if(owner){
        sem_close(data->sem);
        sem_unlink(name[1]);
    }
    munmap(data, sizeof(shm_read));
    close(*shm_fd);
    if(owner) shm_unlink(name[0]);
}

// silly little animation thing
const char* anim = "|/-\\";

// print loading bars on client and server
// ascii escape characters my beloved
void print_bars(State* state, int bar_width, int bar_count){
    printf("\033[s"); // save cursor
    printf("\033[H"); // move cursor to top left
    printf("\033[K"); // clear current line (dont want things flooding past the bar)
    printf("\n\r"); // go to next line and carriage return
    for(int i = 0; i < bar_count; i++){
        // only draw the bar if loading has started
        double d = state[i].load;
        if(d > 0){
            // print slot, tasks completed for slot, time processing (s), percent complete, then a loading bar
            // the loading bar can be scaled, i used to have it on one line but i think multiple lines looks nicer
            printf(" SLOT %i - <%i/%i : %is : %lf%%> - ",i,state[i].tasks,SHIFTS,state[i].time,(d*100));
            printf("[");
            for(int j = 0; j < bar_width; j++){
                char k = '.';
                if(j < d*(bar_width)) k = '|';
                printf("%c",k);
            }
            printf("] %c\n",anim[time(NULL)%4]);
            printf("\033[K"); // clear line below
        }
    }
    printf("\033[u"); // retrieve saved cursor
}

