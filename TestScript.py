# Montana Pawek
# Resources used:

# https://docs.python.org/3/library/subprocess.html#subprocess.run

import subprocess

# Programs to be run
c_programs = ["MatrixMult", "MonteCarlo", "DNS_Resolver"]
python_programs = ["MatrixMult.py", "MonteCarlo.py", "DNS_Resolver.py"]

# This variable determines the number of repetitions for each program
num_runs = 50


if __name__ == "__main__":

    # This variable is used by the DNS resolvers; we used the large.txt file from Assignment 5 and split it into over 200 separate files
    # The number of IP addresses to look up in each file is the variable tested for the DNS Resolvers; there are either 50 names, 250 names, or 500 names per file
    # This variable specifically references the number in the name of the file; e.g. names{this number}.txt
    # We have names1.txt to names300.txt
    # names1.txt - names100.txt contain 50 strings per file
    # names101.txt - names200.txt contain 250 strings per file
    # names201.txt - names300.txt contain 500 strings per file
    input_file_count = 0

    # For each program name stored in these arrays:
    for program in c_programs + python_programs:
        # If the program name ends in .py, it's python
        is_python = program.endswith(".py")

        # Loop to run each program the total num_runs times
        for i in range(num_runs):
            # Check if the current program is a python or C program so we know the proper syntax to run it
            if is_python:
                cmd = ["python", program]
            else:
                cmd = [f"./{program}"]

            # If the program is one of the DNS resolvers, it needs command line arguments
            if "DNS_Resolver" in program:
                # Iterate input_file_count once each loop to use a new names.txt file so the cached DNS doesn't affect the results
                input_file_count += 1
                input_file1 = f"names/names{input_file_count}.txt"
                
                if is_python:
                    output = f"Py_DNS_Results.txt"

                else:
                    output = f"C_DNS_Results.txt"
            
                # Command line arguments
                cmd.extend ([input_file1, output])

            if is_python: 
                # Print statement to keep track of what's running
                print (f"Python {program} # {i + 1}")

            else: 
                # Print statement to keep track of what's running
                print (f"C {program} # {i + 1}")

            # Run the process
            # First argument is the program to be run, the second argument allows this program to capture stdout and stderr in case of errors,
            # The third argument also allows us to open file objects which I believe is necessary, the fourth argument specifies to look at this specific directory for input/output files
            result = subprocess.run (cmd, capture_output = True, text = True, cwd = ".")

            # Error checking: If a process cannot run with given commands, print error message
            if result.returncode != 0:
                print (f"Error running {program}: {result.stderr}")    
