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
#include "randomizer.cpp"

// Define any constants or macros
bool do_show_output = false;
bool do_show_pagetable = false;
bool do_show_frametable = false;
bool do_show_stats = false;
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

struct global_stats{
    unsigned long inst_count;
    unsigned long ctx_switches;
    unsigned long process_exits;
    global_stats() : inst_count(0), ctx_switches(0), process_exits(0) {}
};

struct process_stats{
    unsigned long unmaps;
    unsigned long maps;
    unsigned long ins;
    unsigned long outs;
    unsigned long fins;
    unsigned long fouts;
    unsigned long zeros;
    unsigned long segv;
    unsigned long segprot;
    process_stats() : unmaps(0), maps(0), ins(0), outs(0), fins(0), fouts(0), zeros(0), segv(0), segprot(0) {}
};

struct process_object{
    pte_t page_table[MAX_VPAGES]= {0,0,0,0,0,0};
    int process_id;
    int number_of_VMA;
    std::vector<VMA> VMA_list;
    process_stats pstats;
    process_object(std::vector<VMA> VMA_list_input) : VMA_list(VMA_list_input), pstats(process_stats()) {}
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

class Random : public pagerClass {
    public:
    Randomizer randomizer;
    Random(int n_f, Randomizer& _randomizer) : pagerClass("Random", n_f), randomizer(_randomizer) {}
    int hand;
    frame_t* select_victim_frame(frame_t* frame_table) override;
};

#endif // MMU_H
