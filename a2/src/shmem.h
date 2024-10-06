#ifndef SHMEM_H
#define SHMEM_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned long long int
#define BUFFER 128
#define POOL 12
#define INPUT 64
#define OUTPUT 256
#define LINES 32

// should make it clear that i could NOT get semaphores to compile in cygwin, so im using shmem mutexes
typedef struct shm_read {
    pthread_mutex_t lock;
    u32 clientslot;
    u8 clientflag;
    u8 requests;
    u32 slot[POOL];
    u8 load[POOL];
    u8 flag[POOL];
} shm_read;

typedef struct task {
    u32 integer;
    void* slot;
} task;

shm_read* shm_create(int* shm_fd, char* shm, u8);
void shm_destroy(int* shm_fd, char* shm, u8);

#endif