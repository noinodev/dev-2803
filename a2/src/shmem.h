#ifndef SHMEM_H
#define SHMEM_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/sem.h>

// i hate cygwin i hate cygwin

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned long long int
#define BUFFER 128
#define POOL 12
#define SHIFTS 32
#define INPUT 64
#define OUTPUT 256
#define LINES 32
#define SLOT_EMPTY 0
#define SLOT_WORKING 1
#define SLOT_READY 2
#define SLOT_FINISHED 3
#define SLOT_BUSY 255

typedef struct State{
    double load;
    int time;
    int tasks;
    int tasks_total;
} State;

// should make it clear that i could NOT get semaphores to compile in cygwin, so im using shmem mutexes
// ADDENDUM cygwin environment doesnt support shmem mutexes which is LOVELY because i had to start again and eventually figure out how to get semaphore.h to work
typedef struct shm_read {
    //pthread_mutex_t lock;
    sem_t* sem;
    //sem_t* sem_pool[POOL];
    //sem_t* sem_update[POOL];
    volatile u32 clientslot;
    volatile u8 clientflag;
    volatile u8 serverflag[POOL];
    volatile u32 slot[POOL];
    //volatile double load[POOL];
    volatile State state[POOL];
} shm_read;

/*typedef struct task {
    u32 integer;
    void* slot;
} task;*/

shm_read* shm_create(int* shm_fd, u8);
void shm_destroy(int* shm_fd, shm_read* shm, u8);
void print_bars(State* state, int bar_width, int bar_count);

#endif