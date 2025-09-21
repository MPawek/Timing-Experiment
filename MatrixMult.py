# Montana Pawek
# Resources used:

    # https://github.com/khansaadbinhasan/Parallel-Programming-MultiProcessing-in-Python/blob/master/mp_mat_mult.py
    # https://www.geeksforgeeks.org/multiprocessing-python-set-1/
    # https://www.geeksforgeeks.org/multiprocessing-python-set-2/
    # https://www.geeksforgeeks.org/synchronization-pooling-processes-python/
    # https://www.geeksforgeeks.org/python-program-multiply-two-matrices/
    # https://www.digitalocean.com/community/tutorials/python-multiprocessing-example
    # https://docs.python.org/3/library/multiprocessing.html
    # https://docs.python.org/3/tutorial/inputoutput.html
    # https://www.w3schools.com/python/ref_func_map.asp
    # https://stackoverflow.com/questions/15414027/multiprocessing-pool-makes-numpy-matrix-multiplication-slower


import random
import multiprocessing as mp
import time

 # We want to figure out the max number of processes we can run based on CPU cores, which we can do with cpu_count ()
 # Size is the variable we change to vary the difficulty of the program. 64, 256, and 512 are the testing sizes
SIZE = 256 
PROCESSES = mp.cpu_count () 

# We can split back out the args in the parameters here due to our use of starmap
def multiply_rows (start_row, end_row, matrixA, matrixB):
    # Originally I considered using the NumPy library which supposedly is much more effective at multiplying matrices,
    # But I decided to keep the code as similar to the C code as possible, as we are measuring multithreading vs. multiprocessing performance,
    # Not which program has the better matrix library

    # Create array to hold result matrix's finished row
    final_rows_result = []

    # For each row assigned to this specific process:
    for i in range (start_row, end_row):
        # Initialize row results array here so it is cleared every new loop
        row_result = []
        
        # Start nested for loop to do the matrix multiplication
        for j in range(SIZE):
            # Initialize total here so it always resets to zero once we move on to the next cell
            total = 0

            # Determine final value
            for k in range(SIZE):
                total += matrixA[i][k] * matrixB[k][j]

            # Set result matrix cell to final total, then start the next loop
            row_result.append (total)
        
        # Store the full results in the row array
        final_rows_result.append ((i, row_result))

    # Return the matrix multiplied rows
    return final_rows_result


if __name__ == '__main__':
    # Generate random seed based on current time
    random.seed (time.time ())

    # Divide rows amongst processes, giving the remainder to the last process
    rows_per_process = SIZE // PROCESSES
    remainder = SIZE % PROCESSES

    # Initialize args array and start variable value
    args = []
    start = 0

    # Fill initial arrays with random numbers between 0 and 100
    # Done with nested for loops for x and y axes
    # Fill results array with zeroes
    matrixA = [[random.randint(0, 99) for _ in range(SIZE)] for _ in range(SIZE)]
    matrixB = [[random.randint(0, 99) for _ in range(SIZE)] for _ in range(SIZE)]
    result = [[0] * SIZE for _ in range (SIZE)]

    # Assign rows to processes
    # We created an array of arguments to pass in with each process; this is to pass multiple arguments in to the process' function without needing a shared struct
    # Since the logic is still the same as the C version (and bestcount.c), we shouldn't need locks even if we're using shared memory for multiprocessing as none of them write to the same region of memory
    for i in range (PROCESSES):
        end = start + rows_per_process + (1 if i == PROCESSES - 1 else 0)
        args.append ((start, end, matrixA, matrixB))
        start = end

    # Start timer
    start_time = time.time_ns ()

    # mp.Pool creates a 'pool' of processes, similar to threads
    # Here, it loops through each set of rows, assigning a process to that set. Since we use global values, we don't need the return value that pool.map usually generates
    # Starmap is used as it's better than map for passing this information back and forth
    with mp.Pool (processes = PROCESSES) as pool:
        results = pool.starmap (multiply_rows, args)

    # Reconstruct final result matrix using rows returned from processes,
    # Similar to bestcout.c, we use the return value of the processes instead of modifying the shared data with them
    result = [None] * SIZE
    for final_rows_result in results:
        for row_index, row_data in final_rows_result:
            result[row_index] = row_data

    # End timer and calculate time taken
    end_time = time.time_ns ()
    time_taken = (end_time - start_time) / (1e9)

    # Define and open output file to store results in
    with open ("PyMatrixMultResults.txt", "a") as output:
        output.write (f"{time_taken:.6f}\n")





