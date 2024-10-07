POSIX IPC PRIME FACTOR TRIAL DIVISION PROGRAM
made by Nathan Burg s5348935

Important notes
Cygwin/Windows has severe compatability issues with POSIX IPC / PTHREAD functions: mutexes and condition variables cannot work inter-process.
More specifically the mutex/condition attribute 'pthread_process_shared' causes EINVAL 22 errno for initializing mutexes and condition variables with this attribute.
I also tested them without this attribute with stripped down version of this program, and confirmed inter-process mutexes cannot and will not work at all in Cygwin.
Unless there is a way around it, but the normal POSIX functionality is not supported in this case: if there were a way, it would be obscure and Cygwin-specific, and it would have been nice to know about it in advance.
There are some serious synchronization issues with shared memory in Cygwin as a result of this, which were ultimately insurmountable.
One of these issues was shared memory and threads: when modifying shared memory on another thread, even if the semaphore/mutex is held, the data is never reflected inter-process.
It could be observed between threads on the same process that the memory was being modified, however

Originally I had developed this program to work strictly with mutexes and condition variables, and then I spent nearly 48 consecutive hours trying to debug and/or find a workaround. 
The new solution I used is a simple event scheduler, with a FIFO circular buffer.
My submission is late for this reason alone, though I'm happy to eat the late penalty.

Ultimately I used a regular named semaphore, with no time or energy left to bring it to the same functionality as mutexes and condition variables in the original program, and buffered the server output.
I have a decent understanding of mutexes and condition variables from multithreaded programs I've built for this class and for 2813ICT.
I can guarantee the implementation I had before, if shm mutex/cond were working, would meet the requirements exactly, as in no buffered output.

Building and installation
The programs are built with gcc under a Cygwin environment
-	cd into makefile directory
-	to install locally, use ‘make [client | server | all]
-	to clean local directory, including object and dependency files, use ‘make clean’

Running the program(s) 
-   Run ./bin/server and ./bin/client from project directory
-   Server should always be started first, as it owns the shared memory. The client will segfault if started first.

Interacting with the program(s)
-   On the client, you can enter an integer:
        Any real number >0 will be pushed to the server for processing.
        Entering 0 will put the server into test mode, if the server is not already busy.
-	In either the client or server, typing 'quit' will gracefully close the program. 
        Shared memory and named semaphores aren't deallocated at program close, so this is highly advised.
-   In any event where the server crashes or is interrupted, to unlink the semaphores and shared memory you should run the server again, and type 'quit'
