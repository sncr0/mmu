Virtual Memory Management Simulation

This project is an implementation of a virtual memory management simulation for the Operating Systems course (CSCI-GA.2250-001, Fall 2023) at NYU. The simulation involves handling virtual memory addresses, page tables, and page replacement algorithms.
Instructions

The program takes input instructions from a file and simulates the behavior of a virtual memory system. Each instruction represents a memory operation, such as reading or writing to a virtual page. The instructions are in the following format:

    c <procid>: Context switch to process with ID <procid>.
    r <vpage>: Read from virtual page <vpage>.
    w <vpage>: Write to virtual page <vpage>.
    e <procid>: Exit the current process with ID <procid>.

A sample instruction sequence looks like this:

r

c 0
r 32
w 9
r 0
r 20
r 12

Data Structures

The implementation uses process objects, each with its array of virtual memory areas (VMAs) and a page table representing translations from virtual pages to physical frames for that process. Each page table entry (PTE) contains the PRESENT/VALID, REFERENCED, MODIFIED, WRITE_PROTECT, and PAGEDOUT bits, along with the number of the physical frame.
Page Replacement Algorithms

The simulation includes the implementation of various page replacement algorithms, including FIFO, Random, Clock, Enhanced Second Chance / NRU, Aging, and Working Set. These algorithms are implemented as derived classes of a general Pager class.
Usage

bash

./mmu -f<num_frames> -a<algo> [-o<options>] inputfile randomfile

    <num_frames>: Number of physical frames.
    <algo>: Page replacement algorithm (e.g., c for Clock, r for Random).
    <options>: Additional options for output formatting (e.g., O for output, P for pagetable, S for statistics).

Options

    O: Generate detailed output as specified in the example outputs.
    P: Print the state of the page table after executing all instructions.
    F: Print the state of the frame table after executing all instructions.
    S: Print per-process statistics and a summary line.
    x: Print the current page table after each instruction.
    y: Print the page table of all processes after each instruction.
    f: Print the frame table after each instruction.
    a: Print additional aging information during victim selection and after each instruction for complex algorithms.

Example Invocation

bash

./mmu -f4 -ac -oOPFS inputfile randomfile

This command selects the Clock algorithm, creates output for operations, pagetable content, frame table content, and summary.
Deductions

    Modular Design: A lack of modular design with separated replacement policies and simulation leads to a deduction of 5pts.
    Initializing PTE: Initializing the PTE before instruction simulation to anything other than "0" results in a deduction of 5pts.
    Printing Size of PTE: Not printing the real size of pte_t from the program will cost 2pts.
    Organization of Data Structures: Incorrect organization of pagetable and frame table structures will result in a deduction of 2pts each.

Page Table and Frame Table Content Example

This section provides an example of the page table and frame table content after executing all instructions, as requested by the 'P' and 'F' options.
Page Table Content

bash

PT[0]: 0:RMS 1:RMS 2:RMS 3:R-S 4:R-S 5:RMS 6:R-S 7:R-S 8:RMS 9:R-S 10:RMS 
11:R-S 12:R-- 13:RM- # # 16:R-- 17:R-S # # 20:R-- # 22:R-S 23:RM- 24:RMS # # 
27:R-S 28:RMS # # # # # 34:R-S 35:R-S # 37:RM- 38:R-S * # 41:R-- # 43:RMS 
44:RMS # 46:R-S * * # * * * # 54:R-S # * * 58:RM- * * # * * 

    R (referenced), M (modified), S (swapped out).
    #: PTEs that are not valid and have been swapped out.
    *: PTEs that do not have a swap area associated with.
    -: Bits not set.

Frame Table Content

bash

FT: 0:32 0:42 0:4 1:8 * 0:39 0:3 0:44 1:19 0:29 1:61 * 1:58 0:6 0:27 1:34

    <pid:vpage>: Process ID and virtual page mapping.
    *: Frames not currently mapped by any virtual page.