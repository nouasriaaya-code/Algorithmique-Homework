# Sorting Algorithms Assignment

Université Badji Mokhtar - Annaba  
Computer Science Department  
Module: Algorithms and Data Structures  
Instructor: Dr. Sabri Ghazi  
Author: nouasria aya 
Group: 1  
Academic year: 2026/2027  
Programming language: c
## Project overview

This project implements five sorting algorithms from scratch: Bubble Sort, Selection Sort, Insertion Sort, Merge Sort and Quick Sort. Each algorithm is implemented as an independent function. The sorting functions are kept separate from the menu, input/output and benchmark code.

The program includes manual array input, original and sorted array display, CPU-time measurement, operating-system peak-memory measurement, repeated tests, a multi-size comparison, CSV export and a large-array example with 100,000 integers.

No built-in sorting function is used.

## Libraries and APIs

Standard C headers:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
```

Platform-specific headers used only for operating-system memory measurement:

```c
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif
```

- `stdio.h`: keyboard input, screen output and CSV files.
- `stdlib.h`: dynamic temporary memory for Merge Sort, random number generation and process execution.
- `string.h`: `memcpy` to copy the same original array before each measured run.
- `stdbool.h`: Boolean values for input validation and Bubble Sort's early-exit flag.
- `math.h`: included among the project headers; no mathematical function is needed by these sorting algorithms.
- `time.h`: `clock()` and `CLOCKS_PER_SEC` for processor-time measurement.
- Windows: `GetProcessMemoryInfo()` from `psapi.h` obtains the worker process's `PeakWorkingSetSize`.
- Linux/POSIX: `getrusage(RUSAGE_SELF, ...)` obtains `ru_maxrss`.

Every measured run is executed in a separate worker process. This keeps the operating-system peak-memory counter for one algorithm run separate from previous algorithms. The worker loads and copies the test array before starting the timer. Only the sorting function is inside the timed interval. The process peak includes the worker program, its input array and the algorithm's memory while that worker is running.

## Compile and run

Run the commands from the repository root so the `results/` directory is available.

### Windows with MSYS2 UCRT64

```bash
gcc -std=c11 -O2 -Wall -Wextra src/main.c -o sorting.exe -lpsapi
./sorting.exe
```

### Linux

```bash
gcc -std=c11 -O2 -Wall -Wextra src/main.c -o sorting
./sorting
```

The program uses the Windows process-memory API on Windows and `getrusage()` on Linux. The Windows build must link `psapi` with `-lpsapi`.

## Interactive menu

| Option | Action |
|---:|---|
| 1 | Bubble Sort |
| 2 | Selection Sort |
| 3 | Insertion Sort |
| 4 | Merge Sort |
| 5 | Quick Sort |
| 6 | Compare all five algorithms over several sizes and four input types |
| 7 | Run Merge Sort and Quick Sort on 100,000 random integers |
| 0 | Exit |

For options 1 to 5, enter an array size between 1 and 100, then enter the elements manually. The program displays the original array, the sorted array, average CPU time over five independent runs and peak process memory reported by the operating-system API.

The menu continues until the user chooses 0. Invalid menu choices and invalid integer input are handled without terminating the program.

## Algorithms

### Bubble Sort

Compares adjacent elements and swaps them when they are in the wrong order. A `swapped` flag stops the algorithm early when a complete pass makes no swaps.

| Best time | Average time | Worst time | Auxiliary space |
|---:|---:|---:|---:|
| O(n) | O(n^2) | O(n^2) | O(1) |

### Selection Sort

Finds the minimum element in the unsorted part of the array and swaps it with the first unsorted position.

| Best time | Average time | Worst time | Auxiliary space |
|---:|---:|---:|---:|
| O(n^2) | O(n^2) | O(n^2) | O(1) |

### Insertion Sort

Takes each element and inserts it into its correct position in the already sorted left part of the array.

| Best time | Average time | Worst time | Auxiliary space |
|---:|---:|---:|---:|
| O(n) | O(n^2) | O(n^2) | O(1) |

### Merge Sort

Recursively divides the array into two halves, sorts each half and merges the sorted halves. It uses one temporary array.

| Best time | Average time | Worst time | Auxiliary space |
|---:|---:|---:|---:|
| O(n log n) | O(n log n) | O(n log n) | O(n) |

### Quick Sort

Uses the median of the first, middle and last elements as the pivot. A three-way partition separates values smaller than, equal to and greater than the pivot. The implementation recursively processes the smaller partition and iterates over the larger partition to limit call-stack depth.

| Best time | Average time | Worst time | Auxiliary space |
|---:|---:|---:|---:|
| O(n log n) | O(n log n) | O(n^2) | O(log n) stack depth |

## Time and memory measurement

### Execution time

The program uses the standard C `clock()` function. The measured interval contains only the selected sort function:

```c
clock_t start = clock();
sortFunction(workArray, size);
clock_t end = clock();

double milliseconds =
    1000.0 * (double)(end - start) / CLOCKS_PER_SEC;
```

Each measurement is repeated five times in independent worker processes. The average CPU time is reported. Array loading, copying, process startup, correctness checking and printing are outside the timed interval. Very small arrays may display `0.000 ms` because the operation can be shorter than the timer resolution.

### Peak memory

On Windows, `GetProcessMemoryInfo()` reads `PeakWorkingSetSize`. On Linux, `getrusage()` reads `ru_maxrss`. Each sorting run is isolated in a worker process so that the peak counter is not carried over from a different sorting algorithm. The largest peak reported across the five runs is written in KB.

The reported value is the peak memory of the worker process, including its runtime and input array, while it performs that sorting run. It is an operating-system measurement, not an estimate based only on the size of an auxiliary array.

## Experimental comparison

Option 6 compares these array sizes:

| Array size | Bubble | Selection | Insertion | Merge | Quick |
|---:|---|---|---|---|---|
| 1,000 | Run | Run | Run | Run | Run |
| 5,000 | Run | Run | Run | Run | Run |
| 10,000 | Skipped | Skipped | Skipped | Run | Run |
| 50,000 | Skipped | Skipped | Skipped | Run | Run |
| 100,000 | Skipped | Skipped | Skipped | Run | Run |

Bubble Sort, Selection Sort and Insertion Sort are limited to 5,000 elements because their average and worst-case time complexities are quadratic. Merge Sort and Quick Sort are tested at all five sizes.

Four input types are tested:

1. Random integers.
2. Already sorted integers.
3. Reverse-sorted integers.
4. Integers with many duplicates.

A fixed random seed (`2026`) makes generated inputs reproducible. For a given input type and array size, every algorithm receives a copy of the same original data. Each measurement is repeated five times. Rank 1 means the lowest average CPU time among the algorithms that were run for that particular input type and size.

Option 6 creates `results/comparison.csv`. The CSV contains the algorithm, input type, array size, average CPU time, peak process memory, recursion depth, rank and run status.

## Large-array example

Option 7 generates at least 100,000 random integers using the fixed seed `2026`. Merge Sort and Quick Sort run on separate copies of the same data. The program verifies that each result is sorted and displays the first and last ten values. Bubble Sort, Selection Sort and Insertion Sort are skipped for this large example because their quadratic running times can be excessive.

The large-array measurements are saved to `results/big_array.csv`.

## Results and machine specifications

Run options 6 and 7 on the computer used for the assignment. Keep the generated `results/comparison.csv` and `results/big_array.csv` files in the repository. Do not use benchmark values from another computer as though they were measured on your own computer.

Fill in the actual machine specifications after running the program:

- CPU: [Enter CPU model]
- RAM: [Enter installed RAM]
- Operating system: [Enter OS and version]
- Compiler: [Enter GCC version]

## Critical analysis

The theoretical analysis predicts that Bubble Sort and Insertion Sort can be efficient on already sorted or nearly sorted arrays because their best-case running time is O(n). Their running time grows quadratically on reverse-sorted or random data. Selection Sort performs a quadratic number of comparisons even when the input is already sorted.

Merge Sort has O(n log n) running time in its best, average and worst cases, so its growth should remain more stable as the input size increases. It requires O(n) auxiliary array memory.

Quick Sort is designed here with a median-of-three pivot and three-way partitioning. These choices help avoid poor partitions on sorted, reverse-sorted and duplicate-heavy inputs. Its average running time is O(n log n), while the theoretical worst case remains O(n^2). Recursing on the smaller partition limits the call-stack depth.

The actual fastest algorithm and measured memory values depend on the computer, compiler, optimization options and operating system. After running the benchmarks, use the CSV files to complete the observations below with the actual measured values:

- Fastest algorithm for each tested input type and size: [Complete from comparison.csv]
- How Merge Sort and Quick Sort changed as the array size increased: [Complete from comparison.csv]
- Whether the quadratic algorithms showed the expected growth on the tested sizes: [Complete from comparison.csv]
- Differences between theoretical and observed results: [Explain using your measurements]
- Problems or limitations encountered during testing: [Complete]

## GitHub submission checklist

1. Keep the repository public.
2. Include `src/main.c`, `README.md`, `.gitignore` and the generated CSV result files.
3. Run the benchmarks on the assignment computer and fill in its CPU, RAM, operating system and compiler details.
4. Complete the critical analysis using the generated results.
5. Open the repository in a private/incognito browser window to confirm that it is public and that all files are accessible.
6. Submit the repository link on the e-learning platform.
