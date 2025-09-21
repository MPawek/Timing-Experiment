/*
Montana Pawek
Resources used:
    Example Files in Assignment 5 Folder:
        lookup.c
    Shared Files in OneDrive:
        Pthreads folder:
            hello.c
            hello_arg2.c
            join.c
        pthreadshared folder:
            goodcount.c
            bettercount.c
            bestcount.c
        IPC folder:
            shm_mutex.c
    Edstem Files:
        producerconsumer_many.c
    Man Pages:
        pthread_mutex_init
        pthread_mutex_lock
        pthread_cond_t
        pthread_cond_wait
        pthread_cond_init
        pthread_cond_signal
        pthreads
        pthread_create
        pthread_join
        pthread_mutex_init
        pthread_mutex_lock
        usleep
        random
        valgrind
*/
#include <pthread.h>

#define MAX_NAME_LENGTH 1025
#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2

// Struct example borrowed from lecture, modified with Assignment 6 conditional variables
struct shared_variables
{
    // Variables to hold the command-line arguments
    int argc;
    char** argv;

    // Variables involved with thread process
    int count;                                               // Counts number of strings present in buffer; prevents trying to add more strings when buffer is full
    char shared_buffer[MAX_INPUT_FILES][MAX_NAME_LENGTH];    // Buffer, holds addresses to look up; 1025 is the string limit
    pthread_mutex_t buffer;                                  // Mutex lock for the buffer; any portion that adds/removes strings from the buffer uses this
    pthread_mutex_t results;                                 // Mutex lock for the results file; any portion that adds strings to the results file uses this
    int requesterDone;                                       // Flag for requester to trip when it's finished; prevents resolver from waiting forever for nonexistent requester to fill empty buffer

    int head;                                                // Pointer to keep track of where in buffer we can input strings
    int tail;                                                // Pointer to keep track of where in buffer we can extract strings

    // Other variables we need:
    // File pointers for input/output files
    FILE* inputfp;
    FILE* outputfp;

    // Conditional variables
    pthread_cond_t not_full, not_empty;                             // Conditional variable to replace mutex waiting loops when buffer is full/empty
};

// Member functions
void *requester (void *shared_v);
void *resolver (void *shared_v);
