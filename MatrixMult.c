/*
Montana Pawek
Resources used:
    https://www.programiz.com/c-programming/examples/matrix-multiplication
    https://stackoverflow.com/questions/73955611/multiplying-two-matrixes-in-c
    bestcount.c
    bettercount.c
    goodcount.c
    all in pthreadshared

*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

// Global variables:
// Initialize size variable, as well as pointers to matrices and the resulting matrix; have to do pointer-to-pointer to avoid errors
// While I initially wanted to avoid completely avoid global variables and use a struct, the fact that each thread must also pass in the rows it will do work on caused problems, both for setting up the struct, and some strange mutex errors.
// I had to choose between creating a struct for each separate thread, or using global variables and smaller structs. This should save on memory allocation, and if performed correctly,
// should avoid the need to use mutex in a similar manner as bestcount.c as each thread is modifying a separate row in the matrix and shouldn't have access to the same memory spaces

// NOTE: Size is the variable we change to vary the difficulty of the program. 64, 256, and 512 are the testing sizes
int size = 64;
int **matrixA;
int **matrixB;
int **result;

// Create structure for threads to know what rows they do work on
typedef struct 
{
    int start_row;
    int end_row;

} rowInfo;

// Thread function to compute multiple rows of the result matrix
void* multiply_rows (void* rowID) 
{
    // Convert struct back from a void* to a struct
    rowInfo *rows = (rowInfo*) rowID;

    // For loop to track row #
    for (int i = rows->start_row; i < rows->end_row; i++) 
    {
        // For loop to keep track of result/matrixB column #
        for (int j = 0; j < size; j++)
        {
            // Inner for loop to do the multiplication
            for (int k = 0; k < size; k++) 
            {
                // NOTE: Should not need mutex locks here as each thread has it's own row to do math with, and they should not be accessing the same memory space except to read
                result[i][j] += matrixA[i][k] * matrixB[k][j];
            }
        }
    }

    // Free malloc'd row memory
    free (rows);

    // Return when finished
    pthread_exit (NULL);
}

// Function to allocate space for a matrix
int** allocate_matrix (int size) 
{
    // Pointer-to-pointer memory allocated w/ malloc
    int** mat = malloc (size * sizeof(int*));

    // For loop that allocates memory to each matrix w/ malloc
    for (int i = 0; i < size; i++)
        mat[i] = malloc (size * sizeof(int));

    // Return pointer
    return mat;
}

// Function to free malloc'd memory
void free_matrix (int** mat, int size) 
{
    for (int i = 0; i < size; i++)
    {
        // Free matrix array memory
        free (mat[i]);
    }
        
    // Free pointer memory
    free (mat);
}

int main () 
{
    // Determine number of CPU cores to find max number of threads
    // Then initialize thread array to hold that many
    // Code from Assignment 5 EC
    int num_threads = sysconf (_SC_NPROCESSORS_ONLN);
    pthread_t threads [num_threads];

    // Calculate how worload is divided by thread, remained will be given to the last thread
    int rows_per_thread = size / num_threads;
    int remainder = size % num_threads;

    // Initialize pthread_create value holder
    int return_status;

    // Allocate and initialize matrix pointers/matrices
    matrixA = allocate_matrix (size);
    matrixB = allocate_matrix (size);
    result = allocate_matrix (size);

    // Initialize srand with time as the seed
    srand (time (NULL));

    // Output file for results:
    FILE* output = fopen ("CMatrixMultResults.txt", "a");
   
    // Make sure results file actually opened:
    if (!output)
    {
        printf ("Error opening file");
        exit (-1);
    }

    // Double for loop that fills both initial matrices and final matrix with random values between 0 and 99, and 0 respectively
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++) 
        {
            matrixA[i][j] = rand () % 100;
            matrixB[i][j] = rand () % 100;
            result[i][j] = 0;
        }
    }

    // Initialize clock struct and start timing
    // Code borrowed from CSCI440 github repo timing.c example
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // For loop to create threads; each thread handles one row of the matrix multiplication
    // This should hopefully avoid the need for mutex locks to increase performance
    for (int i = 0; i < num_threads; i++) 
    {
        // Allocate memory for the struct w/ malloc
        rowInfo *rows = malloc (sizeof (rowInfo));

        // Error checking, make sure malloc works as expected
        if (!rows) 
        {
            fprintf (stderr, "Memory allocation failed\n");
            exit (EXIT_FAILURE);
        }

        // Assign row values based on i
        rows->start_row = (i * rows_per_thread);
        rows->end_row = ((i + 1) * rows_per_thread);

        // The remainder of rows get added to last thread's workload
        if (i == num_threads - 1)
            rows->end_row += remainder;

        // Create thread, thread will call function to multiply row
        // Note that each thread get's its own malloc'd struct, so they are not sharing memory
        return_status = pthread_create (&threads[i], NULL, multiply_rows, rows);

        // Error checking; any value but 0 is the result of a thread generation error
        if (return_status)
        {
            fprintf (stderr, "Requester thread creation error; #%d\n", return_status);
            exit (-1);
        }
    }

    // Join threads after completion
    for (int i = 0; i < num_threads; i++) 
    {
        pthread_join (threads[i], NULL);
    }

    // Finish timer
    clock_gettime (CLOCK_MONOTONIC, &end_time);

    // Calculate time taken
    // NOTE: kept getting negative time results, so we have to modify this part to make sure that doesn't happen
    double time_taken; // = ((end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec)) / 1e9;

    // Error occurs if the end_time.tv_nsec is less than start_time.tv_nsec due to wraparound errors; if statement checks if that's the case
    if (end_time.tv_nsec < start_time.tv_nsec) 
    {
        time_taken = ((end_time.tv_sec - start_time.tv_sec - 1) + (end_time.tv_nsec + 1e9 - start_time.tv_nsec)) / 1e9;
    } 
    
    else 
    {
        time_taken = ((end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec)) / 1e9;
    }

    // Output time to results file
    fprintf(output, "%f\n", time_taken);

    // Need to free memory due to malloc use
    free_matrix(matrixA, size);
    free_matrix(matrixB, size);
    free_matrix(result, size);

    // Close time output file
    fclose (output);

    return 0;
}