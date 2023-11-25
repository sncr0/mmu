#include "mmu.h"


// ===========================|  Pager  |==================================
frame_t* FIFO::select_victim_frame(frame_t* frame_table) {
    frame_t* victim = &frame_table[hand];
    hand = (hand + 1) % num_frames;
    a_output("ASELECT %d\n", victim->id);
    return victim;
}

// ====================|  Diagnostic Functions  |===========================
void printProcessStatistics(process_object* current_process) {
    printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
            current_process->process_id,
            current_process->pstats.unmaps, 
            current_process->pstats.maps, 
            current_process->pstats.ins, 
            current_process->pstats.outs,
            current_process->pstats.fins, 
            current_process->pstats.fouts, 
            current_process->pstats.zeros,
            current_process->pstats.segv, 
            current_process->pstats.segprot);
}

void printGlobalStatistics(std::map<int, process_object> processes, global_stats &gstats) {
    unsigned long long cost = 0;


    cost += (unsigned long long)gstats.inst_count;
    cost += (unsigned long long)gstats.ctx_switches * 130;
    cost += (unsigned long long)gstats.process_exits * 1230;
    for (auto& [id, process] : processes) {
        /* read/write (load/store) instructions count as 1, context_switches instructions=130, process exits instructions=1230.
        In addition if the following operations counts as follows:
        maps=350, unmaps=410, ins=3200, outs=2750, fins=2350, fouts=2800, zeros=150, segv=440, segprot=410 */
        cost += (unsigned long long)process.pstats.maps * 350;
        cost += (unsigned long long)process.pstats.unmaps * 410;
        cost += (unsigned long long)process.pstats.ins * 3200;
        cost += (unsigned long long)process.pstats.outs * 2750;
        cost += (unsigned long long)process.pstats.fins * 2350;
        cost += (unsigned long long)process.pstats.fouts * 2800;
        cost += (unsigned long long)process.pstats.zeros * 150;
        cost += (unsigned long long)process.pstats.segv * 440;
        cost += (unsigned long long)process.pstats.segprot * 410;
    }

    printf("TOTALCOST %lu %lu %lu %llu %lu\n", 
            gstats.inst_count + gstats.ctx_switches + gstats.process_exits, 
            gstats.ctx_switches, 
            gstats.process_exits, 
            cost, 
            sizeof(pte_t));
}

void printPageTable(process_object* current_process) {
    printf("PT[%d]:", current_process->process_id);
    for (int i = 0; i < MAX_VPAGES; ++i) {
        printf(" ");
        /*
        PT[0]: 0:RMS 1:RMS 2:RMS 3:R-S 4:R-S 5:RMS 6:R-S 7:R-S 8:RMS 9:R-S 10:RMS 
        11:R-S 12:R-- 13:RM- # # 16:R-- 17:R-S # # 20:R-- # 22:R-S 23:RM- 24:RMS # # 
        27:R-S 28:RMS # # # # # 34:R-S 35:R-S # 37:RM- 38:R-S * # 41:R-- # 43:RMS 
        44:RMS # 46:R-S * * # * * * # 54:R-S # * * 58:RM- * * # * * 
        R (referenced), M (modified), S (swapped out) (note we don’t show the write protection bit as it is implied/inherited 
        from the specified VMA.
        PTEs that are not valid are represented by a ‘#’ if they have been swapped out (note you don’t have to swap out a 
        page if it was only referenced but not modified), or a ‘*’ if it does not have a swap area associated with. Otherwise 
        (valid) indicates the virtual page index and RMS bits with ‘-‘ indicated that that bit is not set.
        Note a virtual page, that was once referenced, but was not modified and then is selected by the replacement 
        algorithm, does not have to be paged out (by definition all content must still be ZERO) and can transition to ‘*’.
        */
        const pte_t& page_table_entry = current_process->page_table[i];
        if (!page_table_entry.PRESENT) {
            if (page_table_entry.PAGEDOUT) {
                printf("#");
            }
            else {
                printf("*");
            }
        }
        else {
            printf("%d:", i);
            printf("%c", page_table_entry.REFERENCED ? 'R' : '-');
            printf("%c", page_table_entry.MODIFIED ? 'M' : '-');
            printf("%c", page_table_entry.PAGEDOUT ? 'S' : '-');
        }
    }
    printf("\n");
}

void printFrameTable(frame_t* frame_table, int num_frames) {
    printf("FT:");
    for (int i = 0; i < num_frames; ++i) {
        printf(" ");
        const frame_t& frame = frame_table[i];
        if (frame.mapped_pte == nullptr) {
            printf("*");
        }
        else {
            printf("%d:%d", frame.mapped_process->process_id, frame.mapped_vpage);
        }
    }
    printf("\n");
}



void printProcesses(std::map<int, process_object> processes) {
    for (const auto& [id, process] : processes) {
        verbose("Process %d VMA:\n", id);
        for (auto it = process.VMA_list.begin(); it != process.VMA_list.end(); ++it) {
            const auto& [start_vpage, end_vpage, write_protected, id, file_mapped] = *it;
            int index = std::distance(process.VMA_list.begin(), it);
            verbose("  VMA %d: %d %d %d %d\n", index, start_vpage, end_vpage, write_protected, file_mapped);
        }
    }
}


// ====================|  Input Parsing  |===========================
std::string readLine(std::ifstream& file) {
    std::string line;
    while (getline(file, line)){
        if (line[0] == '#') {
            continue;
        }
        else {
            return line;
        }
    }
    return "";
}
std::map<int, process_object> readInput(std::ifstream& file){
    std::string line;
    std::map<int, process_object> processes;
    int num_processes = 0;
    
    num_processes = std::stoi(readLine(file));

    // Check for vma per process in array of processes
    //process_object* processes = new process_object[num_processes];
    for (int i = 0; i < num_processes; i++) {
        int num_vma = std::stoi(readLine(file));
        std::vector<VMA> vma_list;

        for (int j = 0; j < num_vma; j++) {
            std::string line = readLine(file);
            std::istringstream iss(line);
            int start_vpage, end_vpage, write_protected;
            bool file_mapped;
            iss >> start_vpage >> end_vpage >> write_protected >> file_mapped;
            vma_list.push_back(VMA(start_vpage, end_vpage, write_protected, j, file_mapped));
        }
        processes[i] = process_object(vma_list);
        processes[i].process_id = i;
    }
    return processes;
}

void populate_frame_table(int num_frames, std::deque<int> &free_list, frame_t* frame_table) {
    for (int i = 0; i < num_frames; i++) {
        free_list.push_back(i);
        frame_table[i].id = i;
        frame_table[i].mapped_pte = nullptr;
        frame_table[i].mapped_process = nullptr;
        frame_table[i].mapped_vpage = 0;
    }
}

bool get_next_instruction(char* operation, int* vpage, std::ifstream& file) {
    std::string line;
    line =  readLine(file);
    if (line.empty()) {
        return false;
    }
    else {
        std::istringstream iss(line);  
        iss >> *operation >> *vpage;
        return true;
    }
}

// ====================|  Simulation  |===========================
frame_t* allocate_frame_from_free_list(std::deque<int> &free_list, frame_t* frame_table) {
    // allocate a frame from the free list
    // if no free frame is available, return NULL
    // otherwise return a pointer to the frame
    if (free_list.empty()) {
        return nullptr;
    }
    else {
        frame_t* frame_pointer = &frame_table[free_list.front()];
        free_list.pop_front();
        //frame_t* frame = nullptr;
        return frame_pointer;
    }
}

frame_t* get_frame(pagerClass* pager, std::deque<int> &free_list, frame_t* frame_table) {
    // TODO
    frame_t* frame = allocate_frame_from_free_list(free_list, frame_table);
    if (frame == nullptr) frame = pager->select_victim_frame(frame_table);

    /*
    Once a victim frame has been determined, the victim frame must
    be unmapped from its user ( <address space:vpage>), i.e. its entry in the owning process’s page_table must be removed
    (“UNMAP”), however you must inspect the state of the R and M bits. If the page was modified, then the page frame must be
    paged out to the swap device (“OUT”) or in case it was file mapped it has to be written back to the file (“FOUT”). Now the
    frame can be reused for the faulting instruction. First the PTE must be reset (note once the PAGEDOUT flag is set it will
    never be reset as it indicates there is content on the swap device) and then the PTE’s frame must be set and the valid bit can
    be set.
    */

    if (frame->mapped_pte != nullptr){
        pte_t* old_pte = frame->mapped_pte;
        output(" UNMAP %d:%d\n", frame->mapped_process->process_id, frame->mapped_vpage);
        frame->mapped_process->pstats.unmaps++;
        
        if (old_pte->MODIFIED){
            if (frame->mapped_process->VMA_list[frame->mapped_vma_id].file_mapped) {
                output(" FOUT\n");
                frame->mapped_process->pstats.fouts++;
                old_pte->PAGEDOUT = 0;
                old_pte->MODIFIED = 0;
            }
            else {
                output(" OUT\n");
                frame->mapped_process->pstats.outs++;
                old_pte->PAGEDOUT = 1;
                old_pte->MODIFIED = 0;

            }
        } 
    
        old_pte->PRESENT = 0;
        // old_pte->REFERENCED = 0;
        // old_pte->MODIFIED = 0;
        // old_pte->WRITE_PROTECT = 0;
        // old_pte->PHYSICAL_FRAME_NUMBER = 0;
    }
    frame->mapped_pte = nullptr;
    return frame;
}

bool pagefault_handler(process_object* process,
                        int vpage, 
                        pagerClass *pager, 
                        std::deque<int> &free_list,
                        frame_t* frame_table,
                        global_stats &gstats){

    /* OS must resolve:
        • select a victim frame to replace
        (make pluggable with different replacement algorithms)
        • Unmap its current user (UNMAP)
        • Save frame to disk if necessary (OUT / FOUT)
        • Fill frame with proper content of current instruction’s address space (IN, FIN,ZERO)
        • Map its new user (MAP)
        • Its now ready for use and instruction
    */

   /*
    In the pgfault handler you first determine that the vpage can be accessed, i.e. it is part of one of the VMAs. Maybe
    you can find a faster way then searching each time the VMA list as long as it does not involve doing that before the
    instruction simulation (see above, hint you have free bits in the PTE). If not, a SEGV output line must be created and you
    move on to the next instruction.
   */
    bool vpage_in_vma = false;
    VMA* vma_of_vpage = nullptr;
    for (auto it = process->VMA_list.begin(); it != process->VMA_list.end(); ++it) {
        if (vpage >= it->start_vpage && vpage <= it->end_vpage) {
            vma_of_vpage = &(*it);
            vpage_in_vma = true;
            break;
        }
    }
    if (vma_of_vpage == nullptr) {
        process->pstats.segv++;
        printf(" SEGV\n");
        return false;
    }

    /*
    If it is part of a VMA then the page must be instantiated, i.e. a frame must be allocated,
    assigned to the PTE belonging to the vpage of this instruction (i.e. currentproc->pagetable[vpage].frame = allocated_frame ) 
    */

    frame_t* allocated_frame = get_frame(pager, free_list, frame_table);
    process->page_table[vpage].PHYSICAL_FRAME_NUMBER = allocated_frame->id;
    process->page_table[vpage].WRITE_PROTECT = vma_of_vpage->write_protected;
    process->page_table[vpage].PRESENT = 1;
    allocated_frame->mapped_pte = &(process->page_table[vpage]);
    allocated_frame->mapped_process = process;
    allocated_frame->mapped_vpage = vpage;
    allocated_frame->mapped_vma_id = vma_of_vpage->id;
    

    
    /*
    and
    then populated with the proper content. The population depends whether this page was previously paged out (in which case
    the page must be brought back from the swap space (“IN”) or (“FIN” in case it is a memory mapped file). If the vpage was
    never swapped out and is not file mapped, then by definition it still has a zero filled content and you issue the “ZERO” output.
    */
    if (vma_of_vpage->file_mapped == true) {
        output(" FIN\n");
        process->pstats.fins++;
        //process->page_table[vpage].PAGEDOUT = 0;
    }
    else if (process->page_table[vpage].PAGEDOUT) {
        output(" IN\n");
        process->pstats.ins++;
        //process->page_table[vpage].PAGEDOUT = 0;
    }
    else {
        output(" ZERO\n");
        process->pstats.zeros++;
    }
    // if (process->page_table[vpage].PAGEDOUT) {
    //     // bring back from swap space
    //     if (vma_of_vpage->file_mapped == true) {
    //         output(" FIN\n");
    //         process->pstats.fins++;
    //         //process->page_table[vpage].PAGEDOUT = 0;
    //     }
    //     else {
    //         output(" IN\n");
    //         process->pstats.ins++;
    //         //process->page_table[vpage].PAGEDOUT = 0;
    //     }
    // }
    // else {
    //     // zero filled content
    //     output(" ZERO\n");
    //     process->pstats.zeros++;
    // }


    output(" MAP %d\n", allocated_frame->id);
    process->pstats.maps++;

    
    return true;




};

void simulation(int num_frames, std::map<int, process_object> processes, std::ifstream& file, pagerClass* pager) {
    
    frame_t frame_table[num_frames];
    std::deque<int> free_list;
    char operation;
    int vpage;
    process_object* current_process;
    pte_t* pte;	
    int instruction_number = 0;
    global_stats gstats = global_stats();

    populate_frame_table(num_frames, free_list, frame_table);


    while (get_next_instruction(&operation, &vpage, file)) {
        output("%d: ==> %c %d\n", instruction_number, operation, vpage);
        if (operation == 'c') {
            current_process = &processes[vpage];
            gstats.ctx_switches++;
        }
        else if (operation == 'e') {
            /*
            On process exit (instruction), you have to traverse the active process’s page table starting from 0..63 and for each valid entry 
            UNMAP the page and FOUT modified filemapped pages. Note that dirty non-fmapped (anonymous) pages are not written
            back (OUT) as the process exits. The used frame has to be returned to the free pool and made available to the get_frame()
            function again. The frames then should be used again in the order they were released.
            */ //frame->mapped_pte = nullptr;
            printf("EXIT current process %d\n", current_process->process_id);
            gstats.process_exits++;
            for (int i = 0; i < MAX_VPAGES; i++) {
                if (current_process->page_table[i].PRESENT) {
                    output(" UNMAP %d:%d\n", current_process->process_id, i);
                    current_process->pstats.unmaps++;
                    for (auto& frame_table_entry : frame_table) {
                        if (frame_table_entry.mapped_process == current_process && frame_table_entry.mapped_vpage == i) {
                            frame_table_entry.mapped_pte = nullptr;
                            break;
                        }
                    }
                    if (current_process->page_table[i].MODIFIED) {
                        bool vpage_in_vma = false;
                        VMA* vma_of_vpage = nullptr;
                        for (auto it = current_process->VMA_list.begin(); it != current_process->VMA_list.end(); ++it) {
                            if (i >= it->start_vpage && i <= it->end_vpage) {
                                vma_of_vpage = &(*it);
                                vpage_in_vma = true;
                                break;
                            }
                        }
                        if (vma_of_vpage->file_mapped == true) {
                            output(" FOUT\n");
                            current_process->pstats.fouts++;
                        }
                    }
                    free_list.push_back(current_process->page_table[i].PHYSICAL_FRAME_NUMBER);
                }
                current_process->page_table[i].PRESENT = 0;
                current_process->page_table[i].REFERENCED = 0;
                current_process->page_table[i].MODIFIED = 0;
                current_process->page_table[i].WRITE_PROTECT = 0;
                current_process->page_table[i].PAGEDOUT = 0;
                current_process->page_table[i].PHYSICAL_FRAME_NUMBER = 0;
            }
            
        }
        else if (operation == 'r' || operation == 'w') {
            gstats.inst_count++;
            pte = &current_process->page_table[vpage];
            if (!pte->PRESENT) {
                if (!pagefault_handler(current_process, vpage, pager, free_list, frame_table, gstats)){

                    //std::cout << "SEGV" << std::endl;
                    //current_process->pstats.segv++;
                    //exit(1);
                    //gstats.inst_count++;
                    instruction_number++;
                    continue;
                }
            }

            if (operation == 'r') {
                current_process->page_table[vpage].REFERENCED = 1;
            }

            if (operation == 'w') {
                if (current_process->page_table[vpage].WRITE_PROTECT == 1) {
                    std::cout << " SEGPROT" << std::endl;
                    current_process->pstats.segprot++;
                    current_process->page_table[vpage].REFERENCED = 1;
                }
                else {
                    current_process->page_table[vpage].REFERENCED = 1;
                    current_process->page_table[vpage].MODIFIED = 1;
                }
            }

            if (x_flag) {
                printPageTable(current_process);
            }
            if (y_flag) {
                for (auto& [id, process] : processes) {
                    printPageTable(&process);
                }
            }
        }
        else {
            std::cout << "Invalid operation" << std::endl;
            exit(1);
        }
        // handle special case of “c” and “e” instruction
        // now the real instructions for read and write
        //if ( ! pte->present) {
        // this in reality generates the page fault exception and now you execute
        // verify this is actually a valid page in a vma if not raise error and next inst
        //frame_t *newframe = get_frame();
        //-> figure out if/what to do with old frame if it was mapped
        // see general outline in MM-slides under Lab3 header and writeup below
        // see whether and how to bring in the content of the access page.
        //}
        // now the page is definitely present
        // check write protection
        // simulate instruction execution by hardware by updating the R/M PTE bits
        //update_pte(read/modify) bits based on operations.
    
        instruction_number++;
    }
    if (do_show_pagetable) {
        for (auto& [id, process] : processes) {
            printPageTable(&process);
        }
    }
    if (do_show_frametable) {
        printFrameTable(frame_table, num_frames);
    }
    if (do_show_stats) {
        for (auto& [id, process] : processes) {
            printProcessStatistics(&process);
        }
        printGlobalStatistics(processes, gstats);
    }
}

int main(int argc, char **argv) {
    int num_frames = MAX_FRAMES;
    int c;
    char algo = 'f';
    std::string input_file = "../lab3_assign/in1";
    std::string rfile = "rfile";

    /*
     * Input format:
     *  ./mmu –f<num_frames> -a<algo> [-o<options>] inputfile randomfile
     */

    /*
     * Optional arguments:
     */
    while ((c = getopt(argc,argv,"f:a:o:xyfa")) != -1 ){
        switch(c) {
            case 'f':
                num_frames = atoi(optarg);
                break;
            case 'a':
                algo = optarg[0];
                break;
            case 'o':
                if (optarg && *optarg) {
                    for (int i = 0; optarg[i] != '\0'; i++) {
                        switch(optarg[i]) {
                            case 'O':
                                do_show_output = true;
                                break;
                            case 'P':
                                do_show_pagetable = true;
                                break;
                            case 'F':
                                do_show_frametable = true;
                                break;
                            case 'S':
                                do_show_stats = true;
                                break;
                            case 'x':
                                x_flag = true;
                                break;
                            case 'y':  
                                y_flag = true;
                                x_flag = false;
                                break;
                            case 'f':
                                do_verbose = true;
                            case 'a':
                                a_flag = true;
                                break;
                        }
                    }
                }

        }   
    }

    /*
     * Non-optional arguments:
     */
    if ((argc - optind) == 2) {
        input_file = argv[optind];
        rfile = argv[optind+1];
    }
    else if ((argc - optind) == 1) {
        input_file = argv[optind];
    }
    else if ((argc - optind) == 0) {
    }
    else {
        exit(1);
    }

    pagerClass* pager = nullptr;
    if (algo == 'f') {
        pager = new FIFO(num_frames);
    }
    else {
        std::cout << "Invalid algorithm" << std::endl;
        exit(1);
    }
    std::ifstream file(input_file);



    std::map<int, process_object> processes = readInput(file);
    printProcesses(processes);
    simulation(num_frames, processes, file, pager);
}