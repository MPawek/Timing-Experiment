# Timing Experiment: Concurrency and Performance in C and Python

## Overview
This project investigates the **performance tradeoffs between different concurrency models** by comparing implementations written in **C (multithreading with pthreads)** and **Python (multiprocessing)**. Execution time is measured under varying levels of parallelism to evaluate scalability, overhead, and practical limits of each approach.

The project was developed as a final course project with an emphasis on **empirical performance measurement, systems-level reasoning, and controlled experimentation**.

---

## Goals
- Compare concurrency models across languages (C vs Python)
- Measure performance differences between multithreading and multiprocessing
- Observe scaling behavior as parallelism increases
- Understand overhead introduced by synchronization and process management
- Practice designing reproducible performance experiments

---

## Key Features
- Multithreaded implementation in **C using pthreads**
- Multiprocessing implementation in **Python**
- Configurable thread/process counts
- High-resolution timing of computation-only execution paths
- Structured experiments for fair cross-language comparison

---

## Tech Stack
- **Languages:** C, Python  
- **C Concurrency:** POSIX threads (`pthread`)  
- **Python Concurrency:** `multiprocessing` module  
- **Timing:** High-resolution system timers  
- **Platform:** Unix-like systems (Linux preferred)

---

## Academic Context & Code Attribution
This project was completed as an **open-ended final project**, where the instructor provided a broad problem domain and technical scope, but the **specific idea, goals, and experimental direction were defined independently**.

The decision to compare **C pthread-based multithreading** with **Python multiprocessing**, along with the workload design, measurement methodology, and analysis focus, were chosen by the author.

Some source files in this repository are **modified versions of instructor-provided starter code**, supplied as optional scaffolding for the course. These files are clearly labeled at the top of each source file where applicable.

All experimental design choices, performance measurements, comparisons, and conclusions were developed independently on top of this foundation.

---

## Experimental Design

### Workload Structure
Both implementations execute an equivalent computational workload that can be partitioned across multiple threads or processes. Each worker handles a portion of the total work, allowing direct comparison between:
- single-thread/process execution,
- increasing levels of parallelism.

### Concurrency Models
- **C implementation:** Uses pthreads to achieve shared-memory multithreading with low overhead.
- **Python implementation:** Uses multiprocessing to bypass the Global Interpreter Lock (GIL), introducing inter-process communication and higher startup costs.

### Timing Methodology
Execution time is measured using high-resolution timers surrounding only the computation phase. Initialization and teardown are excluded where possible to reduce noise.

Multiple runs are performed to account for variance and improve reliability of results.

---

## Running the Experiments

### Prerequisites
- Unix-like operating system
- C compiler with pthread support (e.g., `gcc`)
- Python 3

### Compile the C Version
```bash
gcc -pthread timing_experiment.c -o timing_experiment
```

### Run the C Version
```bash
./timing_experiment
```

### Run the Python Version
```bash
python3 timing_experiment.py
```

Command-line arguments (when support is implemented) can be used to control:
- number of threads/processes,
- workload size,
- number of repetitions.

---

## Results & Observations
The experiments show:
- **C pthreads scale efficiently** up to hardware core limits due to low threading overhead.
- **Python multiprocessing avoids the GIL** but incurs higher overhead from process creation and IPC.
- Performance gains diminish beyond a certain level of parallelism due to synchronization and system constraints.

These results reinforce that **parallelism introduces tradeoffs**, and language/runtime choice significantly impacts achievable performance.

---

## What This Project Demonstrates
- Systems-level understanding of **concurrency models**
- Practical experience with **pthreads and Python multiprocessing**
- Ability to design and execute **controlled performance benchmarks**
- Awareness of language runtime constraints (e.g., GIL)
- Emphasis on measurement, analysis, and evidence-based conclusions

---

## Limitations & Future Work
- Pin threads/processes to CPU cores to reduce scheduling noise
- Finalize command line agruments to allow user choice at runtime of number of threads/processes, workload size, and number of repetitions
- Automate result collection and visualization
- Extend experiments to additional workloads
- Compare with alternative models (e.g., fork-based parallelism)

---

## Author
Developed independently by **Montana Pawek**.

