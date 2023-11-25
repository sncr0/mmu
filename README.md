
Virtual Memory Management Simulation

  

This project is an implementation of a virtual memory management simulation for the Operating Systems course (CSCI-GA.2250-001, Fall 2023) at NYU. The simulation involves handling virtual memory addresses, page tables, and page replacement algorithms.
 

# Input Specification

  

The input to your program will be comprised of:

  

1. The number of processes (processes are numbered starting from 0).

2. A specification for each process’ address space, comprised of:

	- The number of virtual memory areas/segments (VMAs).

	- Specification for each said VMA, comprised of 4 numbers:

	- Starting virtual page

	- Ending virtual page

	- Write-protected (0/1)

	- File-mapped (0/1)

  

Following is a sample input with two processes. Note: ALL lines starting with ‘#’ must be ignored and are provided simply for documentation and readability. In particular, the first few lines are references that document how the input was created, though they are irrelevant to you. The first line not starting with a ‘#’ is the number of processes. Processes in this sample have 2 and 3 VMAs, respectively. All provided inputs follow the format below, though the number and location of lines with ‘#’ might vary.

  

```plaintext
# process/vma/page reference generator
# procs=2 #vmas=2 #inst=100 pages=64 %read=75.000000 lambda=1.000000
# holes=1 wprot=1 mmap=1 seed=19200
2
#### process 0
2
0 42 0 0
43 63 1 0
#### process 1
3
0 17 0 1
20 60 1 0
62 63 0 0
```
# Data Structures

The implementation employs process objects, each equipped with its array of virtual memory areas (VMAs) and a page table responsible for translating virtual pages to physical frames for that process. Each page table entry (PTE) comprises the PRESENT/VALID, REFERENCED, MODIFIED, WRITE_PROTECT, and PAGEDOUT bits, along with the physical frame number.

# Page Replacement Algorithms

The simulation incorporates the implementation of various page replacement algorithms, such as FIFO, Random, Clock, Enhanced Second Chance / NRU, Aging, and Working Set. These algorithms are realized as derived classes of a general Pager class.

# Usage

```bash
./mmu -f<num_frames> -a<algo> [-o<options>] inputfile randomfile
-   `<num_frames>`: Number of physical frames.
-   `<algo>`: Page replacement algorithm (e.g., c for Clock, r for Random).
-   `<options>`: Additional options for output formatting (e.g., O for output, P for pagetable, S for statistics).
```
 
