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
        pthreads
        pthread_create
        pthread_join
        pthread_mutex_init
        pthread_mutex_lock
        usleep
        random
        valgrind
        pthread_cond_t
        pthread_cond_wait
        pthread_cond_init
        pthread_cond_signal

Skeleton of code based on provided lookup.c file
* File: lookup.c
* Author: Andy Sayler
* Project: CSCI 3753 Programming Assignment 2
* Create Date: 2012/02/01
* Modify Date: 2012/02/01
* Description:
* 	This file contains the reference non-threaded
*      solution to this assignment.
*  
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
 
#include "util.h"
#include "multi-lookup.h"
 
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define INPUTFS "%1024s"

// Function called by first pthread_create; takes strings from input files and loads them into the buffer
void *requester (void *shared_v)
{
    // Recast variable back to struct from void *
    struct shared_variables *sv = (struct shared_variables *) shared_v;

    // Variable used to hold opening of input file error messages
    char errorstr[MAX_NAME_LENGTH];

    // String to hold hostname
    char hostname[MAX_NAME_LENGTH];
    
    // Loop Through Input Files
    for (int i = 1; i < (sv->argc - 1); i++)
    {        
        // Error Check: Open Input File
        // Borrowed from lookup.c
        // Set struct input file pointer to current input file
        sv->inputfp = fopen(sv->argv[i], "r");

        // If input file won't open:
        if(!sv->inputfp){
            sprintf(errorstr, "Error Opening Input File: %s", sv->argv[i]);
            perror(errorstr);
            break;
        }	

        // First critical section; when we put input into bounded buffer
        // While there are more lines to read into the hostname string:
        while (fscanf (sv->inputfp, INPUTFS, hostname) > 0)
        {
            // Try to get lock for the buffer
            pthread_mutex_lock (&sv->buffer);

            // Check to make sure buffer is not full; if it is, wait until it isn't
            while (sv->count >= MAX_INPUT_FILES)
            {
                // Wait until signal is received buffer has space for more names
                // This conditional wait was taken from Assignment 6 code
                pthread_cond_wait (&sv->not_full, &sv->buffer);
            }

            // Copy strings into buffer, use head pointer as it's the first free spot
            strcpy (sv->shared_buffer [sv->head], hostname);
            // Set head pointer to the next available spot; if it's outside the buffer wraparound to the beginning
            sv->head = (++(sv->head) % MAX_INPUT_FILES);

            // Iterate count by 1 to reflect buffer now has one more address
            sv->count++;

            // Signal pthread_wait that buffer has more names to pull
            pthread_cond_signal (&sv->not_empty);

            // After copying to the buffer, unlock
            pthread_mutex_unlock (&sv->buffer);
        }
       
        // Close Input File
        fclose (sv->inputfp);
    }

    // Set requester flag to 1, indicating the requester has finished all it's work and resolver can exit once it's done
    // Critical section; make sure to lock/unlock when changing struct values that other processes will use
    // Note that we use broadcast here to signal all waiting resolver threads instead of just 1
    // Errors would occur where not all resolvers would get the signal and exit
    pthread_mutex_lock (&sv->buffer);
    sv->requesterDone = 1;
    pthread_cond_broadcast (&sv->not_empty);
    pthread_mutex_unlock (&sv->buffer);

    // Exit thread if we reach the end
    pthread_exit (NULL);
}

// Function called by second pthread_create; takes strings from buffer and checks if they're legit. If they are, puts them in results
void *resolver (void * shared_v)
{
    // String to hold buffer string
    char lookupName[MAX_NAME_LENGTH];

    // String to hold IP string
    char firstipstr[INET6_ADDRSTRLEN];

    // Recast variable back to struct from void *
    struct shared_variables *sv = (struct shared_variables *) shared_v;

    // Infinitely loop until requester signals it's done
    while (1)
    {
        // Need to lock the buffer to read from it and remove a string
        pthread_mutex_lock (&sv->buffer);

        // While the buffer is empty but the requester isn't done yet, wait
        while (sv->count < 1 && !sv->requesterDone) 
        {
            // Wait to be signalled the buffer has strings
            // Code taken from Assignment 6
            pthread_cond_wait(&sv->not_empty, &sv->buffer);
        }

        // Once out of the wait loop, check to see if the requester has finished and the buffer is empty. If it has and if it is, exit
        if (sv->count < 1 && sv->requesterDone) 
        {
            pthread_mutex_unlock (&sv->buffer);
            pthread_exit (NULL);
        }

        // Take string from buffer, targeting oldest existing name at the tail, and copy into local variable
        strcpy (lookupName, sv->shared_buffer[sv->tail]);
        // Mark the spot we removed the string from the buffer as ready to be filled
        sv->shared_buffer[sv->tail][0] = '\0';  
        // Set tail pointer to the next available spot; if it's outside the buffer wraparound to the beginning
        sv->tail = (++(sv->tail) % MAX_INPUT_FILES);
                
        // Decrement count to reflect one less address in buffer
        sv->count--;

        // Signal that the buffer has open spaces that can be filled
        pthread_cond_signal (&sv->not_full);

        // Done checking the buffer, unlock it
        pthread_mutex_unlock (&sv->buffer);
        
        // Lookup code borrowed from lookup.c
        // Lookup the hostname and get IP string
        if(dnslookup (lookupName, firstipstr, sizeof(firstipstr)))
        {
            // Error Check: Lookup error
            fprintf (stderr, "dnslookup error: %s\n", lookupName);
            fflush (sv->outputfp);
            strncpy (firstipstr, "", sizeof(firstipstr));
        }
        

        // Now we need to write results to the results file, so we attempt to acquire the lock
        pthread_mutex_lock (&sv->results);

        // Write to Output File, flush to make sure it happens immediately
        fprintf (sv->outputfp, "%s,%s\n", lookupName, firstipstr);
        fflush (sv->outputfp);

        // After writing, unlock
        pthread_mutex_unlock (&sv->results);
    }

    // If thread somehow reaches here (it shouldn't ever), exit
    pthread_exit(NULL);
}
 
int main(int argc, char* argv[])
{
    // Initialize struct
    struct shared_variables sv;

    // Initialize shared_buffer first chars to zero so code works
    for (int q = 0; q < 10; q++)
    {
        sv.shared_buffer[q][0] = 0;
    }

    // Inititalize sv variables
    sv.argc = argc;
    sv.argv = argv;
    sv.count = 0;
    sv.requesterDone = 0;
    sv.head = 0;
    sv.tail = 0;

    // Initialize thread pointer variables
    // Only need one producer thread pointer, as we only need one requester
    // We use an array for the consumer thread pointers, as we will be using a minimum of two resolvers
    // return_value holds pthread_create value to check for errors
    pthread_t p_thread;
    pthread_t c_threads[MAX_RESOLVER_THREADS];
    int return_value;

    // Initialize both mutex locks, and conditional lock
    pthread_mutex_init(&sv.buffer, NULL);
    pthread_mutex_init(&sv.results, NULL);
    pthread_cond_init(&sv.not_full, NULL);
    pthread_cond_init(&sv.not_empty, NULL);

     
    // Error Check: Check Arguments 
    // Borrowed from lookup.c
    // Check to make sure we have the proper number of command line arguments
    if(argc < MINARGS)
    {
        fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
        fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
        return EXIT_FAILURE;
    }
 
    // Error Check: Open Output File
    // Borrowed from lookup.c
    // Check to make sure we can open the output file to store results
    // Set struct output file pointer
    sv.outputfp = fopen(argv[(argc-1)], "w");

    // If we can't open the file:
    if(!sv.outputfp)
    {
        perror("Error Opening Output File");
        return EXIT_FAILURE;
    }

    // Output file for time results:
    FILE* time_output = fopen ("C_DNSResolver.txt", "a");
    
    // Make sure time results file actually opened:
    if (!time_output)
    {
        printf ("Error opening file");
        exit (-1);
    }

    // Calculate time taken; initialize clock and start timer
    // Code borrowed from CSCI440 github repo timing.c example
    struct timespec time_start, time_end;
    clock_gettime (CLOCK_MONOTONIC, &time_start);

    // Create Threads       
    // Check for error when making threads; pthread_create passes 0 back if threads were created successfully
    // Inspiration taken from hello.c example file
    // First argument: Pointer to created thread.
    // Second argument: Don't worry about it; NULL
    // Third Argument: Function pointer to function that runs when thread is created
    // Fourth Argument: Argument to function, (void *) t

    // Create our single producer/requester thread
    return_value = pthread_create (&p_thread, NULL, requester, (void *) &sv);

    // Thread creation error checking
    if (return_value)
    {
        fprintf(stderr, "Requester thread creation error; #%d\n", return_value);
        exit(-1);
    }

    // Create second set of threads consumer/resolver threads to read bounded buffer and try to lookup 
    for (int m = 0; m < MAX_RESOLVER_THREADS; m++)
    {
        return_value = pthread_create (&c_threads[m], NULL, resolver, (void *) &sv);

        // Thread creation error checking
        if (return_value)
        {
            fprintf(stderr, "Resolver thread creation error; #%d\n", return_value);
            exit(-1);
        }
    }
    
    // Join requester thread
    pthread_join (p_thread, NULL);

    // Join resolver threads
    for (int n = 0; n < MAX_RESOLVER_THREADS; n++)
    {
        pthread_join(c_threads[n], NULL);
    }

    // Finish timer as work is done
    clock_gettime (CLOCK_MONOTONIC, &time_end);

    // Calculate time taken
    // NOTE: kept getting negative time results, so we have to modify this part to make sure that doesn't happen
    double time_taken; // = ((end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec)) / 1e9;

    // Error occurs if the end_time.tv_nsec is less than start_time.tv_nsec due to wraparound errors; if statement checks if that's the case
    if (time_end.tv_nsec < time_start.tv_nsec) 
    {
        time_taken = ((time_end.tv_sec - time_start.tv_sec - 1) + (time_end.tv_nsec + 1e9 - time_start.tv_nsec)) / 1e9;
    } 
        
    else 
    {
        time_taken = ((time_end.tv_sec - time_start.tv_sec) + (time_end.tv_nsec - time_start.tv_nsec)) / 1e9;
    }

    // Print time taken to output file
    fprintf (time_output, "%lf\n", time_taken );
 
    // Close Output Files
    fclose (sv.outputfp);
    fclose (time_output);
 
    return EXIT_SUCCESS;
}