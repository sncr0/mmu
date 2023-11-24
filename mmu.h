#ifndef MMU_H
#define MMU_H

// Include any necessary libraries or headers
//#include "verbose.h"
#include "getopt.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <map>
//#include "pager.h"
#include <deque>
#include <vector>

// Define any constants or macros
bool do_show_output = false;
bool do_show_pagetable = false;
bool do_show_frametable = false;
bool do_show_per_proc_stats = false;
bool do_verbose = false;

bool x_flag = false;
bool y_flag = false;
bool f_flag = false;    
bool a_flag = false;

#define MAX_VPAGES 64
#define MAX_FRAMES 128

#define output(fmt...)        do { if (do_show_output) {printf(fmt); fflush(stdout); } } while(0)
#define verbose(fmt...)        do { if (do_verbose) {printf(fmt); fflush(stdout); } } while(0)
#define a_output(fmt...)        do { if (a_flag) {printf(fmt); fflush(stdout); } } while(0)

// Declare any classes, structs, or functions
typedef struct {
    unsigned int PRESENT:1;
    unsigned int REFERENCED:1;
    unsigned int MODIFIED:1;
    unsigned int WRITE_PROTECT:1;
    unsigned int PAGEDOUT:1;
    unsigned int PHYSICAL_FRAME_NUMBER:7;
} pte_t; // can only be total of 32-bit size and will check on this


struct VMA {
    int start_vpage;
    int end_vpage;
    int write_protected;
    int id;
    bool file_mapped;

    VMA(int start_vpage_input, 
        int end_vpage_input, 
        int write_protected_input, 
        int id_input,
        bool file_mapped_input) :
        start_vpage(start_vpage_input),
        end_vpage(end_vpage_input),
        write_protected(write_protected_input),
        id(id_input),
        file_mapped(file_mapped_input) {}

    VMA() {}
};

struct process_object{
    pte_t page_table[MAX_VPAGES]= {0,0,0,0,0,0};
    int process_id;
    int number_of_VMA;
    std::vector<VMA> VMA_list;
    process_object(std::vector<VMA> VMA_list_input) : VMA_list(VMA_list_input) {}
    process_object() {}
    };

typedef struct {
    pte_t *mapped_pte;
    process_object *mapped_process;
    int mapped_process_id;
    int mapped_vpage;
    int mapped_vma_id;
    int id;
} frame_t;

typedef struct name {
    int integer;
    name() : integer(0) {}
} test_struct;

class pagerClass {
public:
    pagerClass(const std::string& scheduler_type, int n_f) : 
        type(scheduler_type),
        num_frames(n_f),
        hand(0) {}
    std::string type;
    int hand;
    int num_frames;
    virtual frame_t* select_victim_frame(frame_t* frame_table) = 0; // virtual base class
};

class FIFO : public pagerClass {
    public:
    FIFO(int n_f) : pagerClass("FIFO", n_f) {}
    int hand;
    frame_t* select_victim_frame(frame_t* frame_table) override;
};

#endif // MMU_H