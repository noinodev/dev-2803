#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <sys/sem.h>
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

    //shm_read* data = //(shm_read*)shm;
    if(owner){
        pthread_mutexattr_t lock_attr;
        int ret;
        ret = pthread_mutexattr_init(&lock_attr);
        if(ret != 0) printf("mutexattr: %s\n",strerror(ret));
        else printf("mutexattr ok\n");
        ret = pthread_mutexattr_setpshared(&lock_attr, PTHREAD_PROCESS_SHARED);
        if(ret != 0) printf("mutexattr_shared: %s\n",strerror(ret));
        else printf("mutexattr_shared ok\n");

        pthread_condattr_t cond_attr;
        ret = pthread_condattr_init(&cond_attr);
        if(ret != 0) printf("condattr: %s\n",strerror(ret));
        else printf("condattr ok\n");
        ret = pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
        if(ret != 0) printf("condattr_shared: %s\n",strerror(ret));
        else printf("condattr_shared ok\n");

        memset(data,0,sizeof(shm_read));
        pthread_mutex_init(&data->lock,&lock_attr);
        for(int i = 0; i < POOL; i++){
            pthread_mutex_init(&data->slotlock[i],&lock_attr);
            pthread_cond_init(&data->slotcond[i],&cond_attr);
        }
        pthread_mutexattr_destroy(&lock_attr);
        pthread_condattr_destroy(&cond_attr);
    }
    return data;
}

void shm_destroy(int* shm_fd, shm_read* data, u8 owner){
    if(owner){
        pthread_mutex_destroy(&data->lock);
        for(int i = 0; i < POOL; i++){
            pthread_mutex_destroy(&data->slotlock[i]);
            pthread_cond_destroy(&data->slotcond[i]);
        }
    }
    munmap(data, sizeof(shm_read));
    close(*shm_fd);
    if(owner) shm_unlink("/SHMEM");
}

