#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include "shmem.h"

shm_read* shm_create(int* shm_fd, u8 owner){
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

    // sempahores
    if(owner){
        memset(data,0,sizeof(shm_read));
        for(int i = 0; i < POOL; i++){

        }
    }
    return data;
}

void shm_destroy(int* shm_fd, shm_read* data, u8 owner){
    if(owner){
        for(int i = 0; i < POOL; i++){
        }
    }
    munmap(data, sizeof(shm_read));
    close(*shm_fd);
    if(owner) shm_unlink("/SHMEM");
}

