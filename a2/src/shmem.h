#ifndef SHMEM_H
#define SHMEM_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/sem.h>

#define u8 unsigned char
#define u32 unsigned int    // type macros for unsigned 8 bit (0-255) and unsigned 32-bit (0-4294967295)
#define POOL 12             // available slots for server
#define SHIFTS 32           // full rotation of 32 bit integer
#define INPUT 64            // input buffer size for fgets

#define SLOT_EMPTY 0        // server slot flags
#define SLOT_WORKING 1
#define SLOT_READY 2
#define SLOT_FINISHED 3
#define SLOT_BUSY 255

// global state struct, server authoritative -> server, shared, client
typedef struct State{
    double load;
    int time;
    int tasks;
    int tasks_total;
} State;

// cygwin environment doesnt support shmem mutexes which is LOVELY because i had to start again and eventually figure out how to get semaphore.h to work
// 1 semaphore seems to work fine, save system resources that way. i used to have a pool of named semaphores but it was just annoying
typedef struct shm_read {
    sem_t* sem;
    volatile u32 clientslot;
    volatile u8 clientflag;
    volatile u8 serverflag[POOL];
    volatile u32 slot[POOL];
    //volatile double load[POOL];
    volatile State state[POOL];
} shm_read;

// shmem.c prototypes
shm_read* shm_create(int* shm_fd, u8);
void shm_destroy(int* shm_fd, shm_read* shm, u8);
void print_bars(State* state, int bar_width, int bar_count);

#endif