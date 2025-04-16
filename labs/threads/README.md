# Matrix Multiplication (Multi-Threading)

## 1. Objectives

- Familiarize with thread programming using the Pthread library.
- Understand the differences between processes and threads.
- Learn to implement parallelized solutions for matrix multiplication.

## 2. Overview

In this lab, It is required to implement a multi-threaded matrix multiplication program.
The goal is to read two matrices, perform matrix multiplication using multi-threading, and compare the performance of three different threading methods.

### Input:
You will read two matrices from text files:  
- Matrix A (of size x*y).
- Matrix B (of size y*z).

### Output:
The resulting matrix C (of size x*z) will be written to output text files for three different threading approaches:
- A thread per matrix.
- A thread per row.
- A thread per element.

---

## 3. Requirements

### Program Execution:
Your program should be executed with the following command:
```
./matMultp Mat1 Mat2 MatOut
```
Where:
- `Mat1` is the input file for matrix A.
- `Mat2` is the input file for matrix B.
- `MatOut` is the prefix for the output file names.

#### Example:
```
./matMultp a b c
```
This will read `a.txt` and `b.txt` and output:
- `c_per_matrix.txt`
- `c_per_row.txt`
- `c_per_element.txt`

If no arguments are provided, default files `a.txt` and `b.txt` will be used, and the output will be:
- `c_per_matrix.txt`
- `c_per_row.txt`
- `c_per_element.txt`

#### Input Format:
Each input matrix file starts with:
```
row=x col=y
```
Followed by the matrix elements. For example:
```
row=3 col=5
1 2 3 4 5
6 7 8 9 10
11 12 13 14 15
```

#### Output Format:
The output files should contain the resulting matrix in the following format:
```
row=2 col=2
1 2
3 4
```
This format will be repeated for all three methods (per matrix, per row, and per element). The values in these matrices will be identical.

### Matrix Multiplication Methods:
You need to implement matrix multiplication using three different threading approaches:
1. **A thread per matrix**: One thread for the entire matrix multiplication.
2. **A thread per row**: One thread for each row of the resulting matrix.
3. **A thread per element**: One thread for each element in the resulting matrix.

You should compare these methods in terms of:
- The number of threads created.
- The execution time taken for each approach.

---

## 4. Synchronization

- **No synchronization functions**: Do not use mutual exclusion (mutexes) or semaphores in your code.
- **Thread Joining**: Do not use `pthread_join` immediately after `pthread_create`. Ensure all threads are created before calling `pthread_join` in the main thread. Misuse of `pthread_join` may result in sequential execution with unnecessary overhead.

---

## 5. Memory Management

Your program must handle memory management carefully. Specifically:
- **Thread Arguments**: For methods where a thread computes per row or per element, you should either:
  - Pass the row/element index as a value.
  - Pass a struct containing the matrix pointers and row/column indices as a reference (if allocated dynamically).

Ensure that all dynamically allocated memory is freed at the end of each worker thread to avoid memory leaks.

---

## 6. Deliverables

- Submit the complete source code.
- Make sure your code is well-commented to explain your approach.
- Measure and output the execution time and number of threads created for each method.

---


## 7. Frequently Asked Questions

### Why is the first method (a thread per matrix) performing better than the others?

- The overhead of creating and managing threads can reduce the benefits of multi-threading, especially for small tasks like matrix multiplication.
-  The first method has the least overhead, which is why it often performs better. In contrast, methods like per row or per element create more threads, increasing the overhead, which may outweigh the benefits for smaller matrix sizes.

---

## 9. Readings & Resources

- [Stack vs Heap Memory](#)
- [C Dynamic Memory Allocation](#)

---
