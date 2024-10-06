#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include "shmem.h"

shm_read* shm_create(int* shm_fd, char* shm, u8 owner){
    *shm_fd = shm_open("/SHMEM",O_CREAT|O_RDWR,0666);
    if (*shm_fd < 0) {
        perror("shm_open");
    }else printf("shm ok\n");
    if(owner) ftruncate(*shm_fd,sizeof(shm_read));
    shm = mmap(0,sizeof(shm_read),PROT_READ|PROT_WRITE,MAP_SHARED,*shm_fd,0);
    if (shm == MAP_FAILED) {
        perror("mmap");
    }else printf("map ok\n");

    shm_read* data = (shm_read*)shm;
    if(owner) pthread_mutex_init(&data->lock,NULL);
    return data;
}

void shm_destroy(int* shm_fd, char* shm, u8 owner){
    shm_read* data = (shm_read*)shm;
    if(owner) pthread_mutex_destroy(&data->lock);
    munmap(shm, sizeof(shm_read));
    close(*shm_fd);
    if(owner) shm_unlink("/SHMEM");
}

