/*
Montana Pawek
Resources used:

   http://stackoverflow.com/a/3067387/1281089
   https://github.com/ishanthilina/Pthreads-Monte-Carlo-Pi-Calculation/blob/master/pthreadTimed.c
   https://github.com/michaelballantyne/montecarlo-pi/blob/master/pi.c
   https://stackoverflow.com/questions/43151361/how-to-create-thread-safe-random-number-generator-in-c-using-rand-r 
   https://cplusplus.com/forum/unices/75447/
   bestcount.c and hello_arg1.c in OneDrive shared example code folders Pthreads and Pthreadshared
   

*/


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> 
#include <sys/time.h>
#include <unistd.h>

// TOT_COUNT is the variable we change to vary the difficulty of the program. 10 million, 50 million, and 100 million are the different tested values
#define TOT_COUNT 100000000    

// Global variable; used to be #define NUM_THREADS but if we're taking in to account the number of threads allowed
// by the system, we have to use global variables as #define needs values to be defined at runtime
// We initialize it to zero here so we can assign value later
int NUM_THREADS = 0;

// We need to generate random numbers for the Monte Carlo Pi estimation, and they must be between 0 and 1
float getRandomNum (int* seed)
{
   // Rand_r must be used as rand() is not thread-safe.
   // Divide random number by max value it could potentially be; this ensures the random number is between 0 and 1
   // We use type conversion to ensure a float is returned
   return rand_r (seed) / (float) RAND_MAX;
}

// The function called when a thread is created to do the Monte Carlo calculations
// NOTE: There are no mutex locks in this program. This is due to the fact we are attempting to use a similar logic as bestcount.c where we use local counters
// and only access the shared counter once for each thread, after it's been joined
// We are attempting this for two reasons: 1 - The function only needs to return an int value, so it should be relatively simple, and 2 - It should speed up the performance
void *monteCarloPi (void *threadID)
{   
   // Convert threadID back to an int, used once below. Had to do (int)(long) as (int) caused errors
   int tid = (int)(long) threadID;    

   // Initialize counter, and allocate malloc'd space for it. Has to be pointer to fit void * return type of function
   int* in_count = malloc (sizeof (int));
   *in_count = 0;
   
   // Divide total number of iterations by number of current threads to figure out how many times each thread should run
   int iterations = TOT_COUNT / NUM_THREADS;
   
   // Generate thread-unique seed for true random numbers, suggested example used clock value to the power of threadID mulitplied by a large number
   int seed = (int)(time (NULL) ^ (tid * 50));

   // The actual calculations
   for (int i = 0; i < iterations; i++)
   {
      // Generate two random floats between 0 and 1
      float x = getRandomNum (&seed);
      float y = getRandomNum (&seed);
      
      // Square each, add them, and then take the square root
      // If the result is less than 1, it's inside the cirle
      if (sqrt ((x * x) + (y * y)) < 1)
      {
         // If inside the circle, iterate the count
         (*in_count)++;
      }
   }
   
   // In the case that the total number of iterations doesn't divide equally amongst number of threads, the first thread will handle the remainder
   // If this is the first thread created:
   if (tid == 0)
   {
      // Find total remaining amount of iterations
      int remainder = TOT_COUNT % NUM_THREADS;
      
      // Same code as above, but for the remainder
      for (int q = 0; q < remainder; q++)
      {
         float x = getRandomNum (&seed);
         float y = getRandomNum (&seed);
         
         float result = sqrt ((x * x) + (y * y));
         
         if (result < 1)
         {
            (*in_count)++;
         }
      }
   }
   
   // Exit thread, returning the count
   pthread_exit (in_count);    
}

int main (int argc, char *argv[])
{
   // Initialize variables; number of cores in computer, thread array, thread creation return value, thread function return value, and aggregate of function return value, respectively
   // Thread count from Assignment 5 EC
   NUM_THREADS = sysconf (_SC_NPROCESSORS_ONLN);
   pthread_t threads[NUM_THREADS];
   int return_status;
   void *value;
   float in_circle = 0;
   

   // Output file for results:
   FILE* output = fopen ("CMonteCarloResults.txt", "a");
   
   // Make sure results file actually opened:
   if (!output)
   {
      printf ("Error opening file");
      exit (-1);
   }

   // Calculate time taken; initialize clock and start timer
   // Code borrowed from CSCI440 github repo timing.c example
   struct timespec time_start, time_end;
   clock_gettime (CLOCK_MONOTONIC, &time_start);
   
   // Loop to create threads
   for (int t = 0; t < NUM_THREADS; t++)
   {
      // Create thread and make it perform the Monte Carlo Pi estimation, have to cast t to long to match pointer sizes
      return_status = pthread_create (&threads[t], NULL, monteCarloPi, (void *) (long)t);

      // Error checking; any value but 0 is the result of a thread generation error
      if (return_status)
      {
         fprintf (stderr, "Requester thread creation error; #%d\n", return_status);
         exit (-1);
      }
   }

   // After threads are done, join them back together
   for (int i = 0; i < NUM_THREADS; i++)
   {     
      // Join threads together, taking value returned from monteCarloPi function
      pthread_join (threads[i], &value);
      
      // Add returned value to total sum of in-circle, casting value to int first
      in_circle += *(int*) value;         
      
      // Free malloc'd memory
      free (value);
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

   // Print timer results to output file
   fprintf (output, "%lf\n", time_taken );

   // Close time file
   fclose (output);
   
   // Exit main
   return 0;
}