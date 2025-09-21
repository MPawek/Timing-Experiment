# Montana Pawek
# Resources used:

# https://www.geeksforgeeks.org/multiprocessing-python-set-1/
# https://www.geeksforgeeks.org/multiprocessing-python-set-2/
# https://www.geeksforgeeks.org/synchronization-pooling-processes-python/
# https://www.digitalocean.com/community/tutorials/python-multiprocessing-example
# https://docs.python.org/3/library/multiprocessing.html
# https://stackoverflow.com/questions/69499387/how-to-implement-multiprocessing-in-monte-carlo-integration


import multiprocessing
import random
import time
import math
import os

 # We want to figure out the max number of processes we can run based on CPU cores, which we can do with cpu_count ()
 # TOT_COUNT is the variable we change to vary the difficulty of the program. 10 million, 50 million, and 100 million are the testing sizes
NUM_PROCESSES = os.cpu_count ()
TOT_COUNT = 100000000

def monteCarloPi (processID):
    # Initialize local_count to zero
    local_count = 0
    # Have to do // for integer division, otherwise iterations becomes a float value and won't work in range
    iterations = (TOT_COUNT // NUM_PROCESSES)

    # Use a seed based on processID and time for true randomness in each process
    random.seed (time.time() * (processID * 5000))

    # For the total number of iterations calculated
    for _ in range (iterations):
        # Assign x and y random values
        x = random.random ()
        y = random.random ()

        # Hypotenuse function does the square root of ((x * x) + (y * y)), if the result is less than 1, iterates the local_count
        if math.hypot (x, y) < 1:  
            local_count += 1

    # Handle remainder of iterations in process 0, same code as above
    if processID == 0:
        remainder = TOT_COUNT % NUM_PROCESSES

        for _ in range (remainder):
            x = random.random ()
            y = random.random ()

            if math.hypot (x, y) < 1:
                local_count += 1

    # Return locally calculated value
    return local_count

if __name__ == '__main__':

    # Create timer variable and start clock
    # Inspiration taken from CSCI 440 Github repo timing.py
    start_time = time.time_ns ()

    # Python uses the multiprocessing library instead of threads
    # Research seems to recommend multiprocessing.pool as it is useful for returning values from the called function
    # Might have to determine max process count allowed by computer and go from there
    # Range will act as a loop counter/processs ID, iterating by one every loop
    # Map is used as it's designed to do work and return a value
    with multiprocessing.Pool (processes = NUM_PROCESSES) as pool:
        value = pool.map (monteCarloPi, range (NUM_PROCESSES))

    # The sum of all returned values is assigned to the in_circle variable
    in_circle = sum (value)
    
    # Pi is calculated
    pi_estimate = 4 * (in_circle / TOT_COUNT)

    # Stop timer, calculate time taken
    end_time = time.time_ns ()
    time_taken = (end_time - start_time) / (1e9)

    # Define and open output file to store results in
    with open ("PyMonteCarloResults.txt", "a") as output:
        output.write (f"{time_taken:.6f}\n")#

