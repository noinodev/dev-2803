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

char sem_name[1+POOL*2][16] = {"/SEM_MAIN"};

shm_read* shm_create(int* shm_fd, u8 owner){
    //shm_unlink("/SHMEM");
    *shm_fd = shm_open("/SHMEM",O_CREAT|O_RDWR,0666);
    if (*shm_fd < 0) {
        perror("shm_open");
        return NULL;
    }else printf("shm ok\n");
    if(owner){
        if(ftruncate(*shm_fd,sizeof(shm_read)) == -1){
            perror("ftruncate");
            close(*shm_fd);
            return NULL;
        }else printf("truncate ok\n");
    }

    shm_read* data = (shm_read*)mmap(0,sizeof(shm_read),PROT_READ|PROT_WRITE,MAP_SHARED,*shm_fd,0);
    if (data == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }else printf("map ok\n");

    if(owner) memset(data,0,sizeof(shm_read));

    // sempahores
    data->sem = sem_open(sem_name[0], O_CREAT|O_EXCL,0666,1);
    if(data->sem == SEM_FAILED) perror("semaphore:");
    else printf("semaphore ok\n");
        /*if(data->sem == 0) perror("sem:");
        for(int i = 0; i < POOL; i++){
            snprintf(sem_name[1+i],sizeof(sem_name[1+i]),"/SEM_POOL_%i",i);
            printf("sem_pool: %s\n",sem_name[1+i]);
            data->sem_pool[i] = sem_open(sem_name[1+i],O_CREAT,0666,1);
            snprintf(sem_name[1+POOL+i],sizeof(sem_name[1+POOL+i]),"/SEM_UPDATE_%i",i);
            data->sem_update[i] = sem_open(sem_name[1+POOL+i],O_CREAT,0666,1);
        }*/
    //}
    return data;
}

void shm_destroy(int* shm_fd, shm_read* data, u8 owner){
    if(owner){
        sem_close(data->sem);
        sem_unlink(sem_name[0]);
        /*for(int i = 0; i < POOL; i++){
            sem_close(data->sem_pool[i]);
            sem_unlink(sem_name[1+i]);
            sem_close(data->sem_update[i]);
            sem_unlink(sem_name[1+POOL+i]);
        }*/
    }
    munmap(data, sizeof(shm_read));
    close(*shm_fd);
    if(owner) shm_unlink("/SHMEM");
}

const char* anim = "|/-\\";

void print_bars(State* state, int bar_width, int bar_count){
    printf("\033[s");
    /*printf("\033[16A");
                for(int i = 0; i < 6; i++){
                    printf("\033[K");
                    printf("\n");
                    printf("\033[K");
                }
    int mov = 0;*/
    printf("\033[H");
    printf("\033[K");
    printf("\n\r");
    for(int i = 0; i < bar_count; i++){
        double d = state[i].load;
        if(d > 0 && state[i].tasks > 0){
            /*if(mov == 0){
                printf("\033[16A");
                for(int i = 0; i < 6; i++){
                    printf("\033[K");
                    printf("\n");
                    printf("\033[K");
                }
            }
            mov = 1;*/
            printf(" SLOT %i - <%i/%i : %is : %lf%%> - ",i,state[i].tasks,SHIFTS,state[i].time,(d*100));
            printf("[");
            for(int j = 0; j < bar_width; j++){
                char k = '.';
                if(j < d*(bar_width)) k = '|';
                printf("%c",k);
            }
            printf("] %c\n",anim[time(NULL)%4]);
            printf("\033[K");
        }
    }
    printf("\033[u");
}

