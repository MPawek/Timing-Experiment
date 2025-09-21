# Montana Pawek
# Resources used:
# Mainly for Python Syntax, Python multiprocessing, Python DNS lookup
    # https://www.geeksforgeeks.org/multiprocessing-python-set-1/
    # https://www.geeksforgeeks.org/multiprocessing-python-set-2/
    # https://www.geeksforgeeks.org/synchronization-pooling-processes-python/
    # https://www.geeksforgeeks.org/python-program-find-ip-address/
    # https://www.digitalocean.com/community/tutorials/python-multiprocessing-example
    # https://www.digitalocean.com/community/tutorials/python-get-ip-address-from-hostname
    # https://docs.python.org/3/library/multiprocessing.html
    # https://docs.python.org/3/library/socket.html
    # https://docs.python.org/3/library/multiprocessing.html#multiprocessing.Process
    # https://docs.python.org/3/library/multiprocessing.html#multiprocessing-programming
    # https://docs.python.org/3/library/sys.html
    # https://docs.python.org/3/tutorial/inputoutput.html
    # https://stackoverflow.com/questions/2575760/python-lookup-hostname-from-ip-with-1-second-timeout
    # https://github.com/codemistic/Data-Structures-and-Algorithms/blob/main/Python%20script%20to%20display%20ip%20address%20and%20host%20name.py
    # https://superfastpython.com/multiprocessing-mutex-lock-in-python/


import multiprocessing as mp
import socket
import sys
import time

# Constants, set to same as C file
MAX_INPUT_FILES = 10
MAX_RESOLVER_PROCESSES = 10
MIN_RESOLVER_PROCESSES = 2


# Shared Object: 
    # Manager allows shared memory between multiple processes
    # buffer is the shared bounded buffer
    # buffer_lock is the lock for the bounded buffer
    # file_lock is the lock for the output file
    # not_full and not_empty are the two conditions for the buffer lock
    # locks required due to critical sections, same as in C program, conditional locks help with requester/resolver
    # Requester_done_flag is the boolean value that is tripped when the requester thread has done all it's work
    # NOTE: Python recommends not using lock.acquire/lock.release manually, as using 'with' with locks automatically releases them when the action has been completed
    # Had to switch conditional lock/buffer to mp instead of manager to avoid errors
class shared_variables:
    def __init__ (self, manager):
        self.buffer = manager.list ()
        self.buffer_lock = manager.Lock ()
        self.file_lock = manager.Lock ()
        self.not_empty = manager.Condition (self.buffer_lock)
        self.not_full = manager.Condition (self.buffer_lock)
        self.done_flag = manager.Value ('b', False)

# NOTE: We use python's DNS lookup feature here because it's super easy to implement, and is certainly easier than figuring out how to
# get python to run util.c or translating util.c to python
def dnslookup (hostname):
    try:
        return socket.gethostbyname (hostname)
    
    # If the lookup doesn't work, mimic the C code logic where it leaves an empty string after the name where the IP address would go
    except socket.gaierror:
        return ""


def requester (sv, input_files):
    # For loop for each file in input_files array
    for file_path in input_files:

        # Try to open file
        try:
            # Open input file as read-only, python file notation suggests using 'with' keyword to make sure file closes properly if error occurs
            with open (file_path, 'r') as infile:

                # Keep reading lines from the file as long as there are lines to read
                for line in infile:

                    # Take string from line w/ whitespaces removed
                    hostname = line.strip ()

                    # Attempt to acquire the buffer_lock if it has the not_full condition (e.g. it has space to put more names in):
                    with sv.not_full:
                        # While the buffer is full, wait
                        while len (sv.buffer) >= MAX_INPUT_FILES:
                            # Unlocks the lock and waits until it's signalled that it can continue
                            sv.not_full.wait ()

                        # Once there's space in the buffer insert the string 
                        sv.buffer.append(hostname)

                        # Notify the not_empty condition there is a string in the buffer
                        sv.not_empty.notify()

        # If we can't open file, print error message:
        except Exception as e:
            print (f"Error reading {file_path}: {e}", file = sys.stderr)

    # Must obtain buffer lock to switch flag to done and indicate requester is finished once all input has been taken
    with sv.buffer_lock:
        sv.done_flag.value = True

    # Before requester exits, it notifies resolvers that they should check the buffer for more strings
    with sv.not_empty:
        sv.not_empty.notify_all()


def resolver (sv, output_file): 
    # Endlessly loop until flag is tripped
    while True:

        # Check conditional variable to make sure we have strings to remove from the buffer
        with sv.not_empty:

            # If buffer is empty and has no strings:
            while not sv.buffer:

                # Check if done_flag is true, if so, we leave the function
                if sv.done_flag.value:
                    return

                # Otherwise, wait until conditional variable signals there are strings in the buffer
                sv.not_empty.wait ()

            # Remove the first string from the buffer
            hostname = sv.buffer.pop (0)

            # After a string has been removed from the buffer, signal there is room in the buffer
            sv.not_full.notify ()

        # Call the dnslookup function on the string to determine IP address
        # Returns IP address is successful, empty string on failure
        ip = dnslookup (hostname)

        # Obtain output file lock, and write results to the output file
        with sv.file_lock:
            with open(output_file, 'a') as f:
                f.write(f"{hostname},{ip}\n")


def main ():
    # Error Check: Check Arguments
    # Make sure we have program call, input file(s), and output file
    if len (sys.argv) < 3:
        print ("Not enough arguments")
        sys.exit (1)
   
    # Initialize input and output files based on command line arguments
    # Reference info: https://docs.python.org/3/library/sys.html
    # Assign input_files all command line arguments after the program name until the last argument, not including the last argument
    # Assign output_file the last command line argument
    input_files = sys.argv[1:-1]
    output_file = sys.argv[-1]

    # Opens outputfile and clears it of previous IPs
    with open (output_file, 'w'):
        pass

    # Initialize manager so processes can share object
    manager = mp.Manager ()
    sv = shared_variables (manager)

    # Start timer
    start_time = time.time_ns ()

    # Create and start requester process, only need 1
    # Arguments: requester = function to be invoked by start/run, args = data to be passed in
    # cont...: including the shared object with the buffer info and locks, and the array of input files we grab names from
    requester_process = mp.Process (target = requester, args = (sv, input_files))
    requester_process.start ()

    # Determine number of resolver processes, and initialize array to hold them
    num_resolvers = MAX_RESOLVER_PROCESSES
    res_procs_array = []

    # Create and start resolver processes
    # For loop that iterates for total number of resolvers being used
    # Arguments: resolver = function to be invoked by start/run, args = data to be passed in 
    # cont...: including the shared object with the buffer info and locks, and the file we output the results to
    # Then, append the current resolver process address to end of the res_procs_array list
    for _ in range (num_resolvers):
        resolver_processes = mp.Process (target = resolver, args = (sv, output_file))
        resolver_processes.start ()
        res_procs_array.append (resolver_processes)

    # Wait for processes to finish, requester first, then join the resolver processes after
    requester_process.join ()
    for resolver_processes in res_procs_array:
        resolver_processes.join ()

    # End timer, calculate time taken
    end_time = time.time_ns ()
    time_taken = (end_time - start_time) / (10 ** 9)

    # Define and open output file to store results in
    with open ("PyDNSResolver.txt", "a") as output:
        # output.write (f"Time taken: {time_taken:.6f} seconds - Estimated Pi: {pi_estimate:.10f}\n")
        output.write (f"{time_taken:.6f}\n")

if __name__ == '__main__':
    # I ran into errors that suggested this code be used for Linux/WSL compatibility
    mp.set_start_method ("fork")  
    main ()