#include "mmu.h"


// ===========================|  Pager  |==================================
frame_t* FIFO::select_victim_frame(frame_t* frame_table) {
    frame_t* victim = &frame_table[hand];
    hand = (hand + 1) % num_frames;
    a_output("ASELECT %d\n", victim->id);
    return victim;
}

frame_t* Random::select_victim_frame(frame_t* frame_table) {
    int random_number = randomizer.myrandom(num_frames);
    frame_t* victim = &frame_table[random_number];
    a_output("ASELECT %d\n", victim->id);
    return victim;
}

frame_t* Clock::select_victim_frame(frame_t* frame_table) {
    frame_t* victim = nullptr;
    while (victim == nullptr) {
        frame_t* frame = &frame_table[hand];
        if (frame->mapped_pte->REFERENCED == 0) {
            victim = frame;
        }
        else {
            frame->mapped_pte->REFERENCED = 0;
        }
        hand = (hand + 1) % num_frames;
    }
    a_output("ASELECT %d\n", victim->id);
    return victim;
}


// ====================|  Diagnostic Functions  |===========================


// Print process statistics for a single process
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


// Print global statistics
void printGlobalStatistics(std::map<int, process_object> processes, global_stats &gstats) {
    
    // Initialize cost to 0
    unsigned long long cost = 0;

    // Calculate cost
    cost += (unsigned long long)gstats.inst_count;
    cost += (unsigned long long)gstats.ctx_switches * 130;
    cost += (unsigned long long)gstats.process_exits * 1230;
    for (auto& [id, process] : processes) {
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

    // Print global statistics
    printf("TOTALCOST %lu %lu %lu %llu %lu\n", 
            gstats.inst_count + gstats.ctx_switches + gstats.process_exits, 
            gstats.ctx_switches, 
            gstats.process_exits, 
            cost, 
            sizeof(pte_t));
}


// Print page table for a single process
void printPageTable(process_object* current_process) {

    // Print page table
    printf("PT[%d]:", current_process->process_id);
    for (int i = 0; i < MAX_VPAGES; ++i) {
        printf(" ");
        pte_t& page_table_entry = current_process->page_table[i];
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


// Print frame table
void printFrameTable(frame_t* frame_table, int num_frames) {
    printf("FT:");

    for (int i = 0; i < num_frames; ++i) {
        printf(" ");
        frame_t& frame = frame_table[i];
        if (frame.mapped_pte == nullptr) {
            printf("*");
        }
        else {
            printf("%d:%d", frame.mapped_process->process_id, frame.mapped_vpage);
        }
    }
    printf("\n");
}


// Print processes
void printProcesses(std::map<int, process_object> processes) {
    for (auto& [id, process] : processes) {
        verbose("Process %d VMA:\n", id);
        for (auto it = process.VMA_list.begin(); it != process.VMA_list.end(); ++it) {
            auto& [start_vpage, end_vpage, write_protected, id, file_mapped] = *it;
            int index = std::distance(process.VMA_list.begin(), it);
            verbose("  VMA %d: %d %d %d %d\n", index, start_vpage, end_vpage, write_protected, file_mapped);
        }
    }
}


// ====================|  Input Parsing  |===========================


// Read info-containing line from a file
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


// Parse input from file
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

// Popule the frame table
void populate_frame_table(int num_frames, std::deque<int> &free_list, frame_t* frame_table) {
    for (int i = 0; i < num_frames; i++) {
        free_list.push_back(i);
        frame_table[i].id = i;
        frame_table[i].mapped_pte = nullptr;
        frame_table[i].mapped_process = nullptr;
        frame_table[i].mapped_vpage = 0;
    }
}


// ====================|  Simulation Helper Functions  |===========================


// Get next instruction from file
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


// Allocate a frame from the free list
frame_t* allocate_frame_from_free_list(std::deque<int> &free_list, frame_t* frame_table) {

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

    frame_t* frame = allocate_frame_from_free_list(free_list, frame_table);
    if (frame == nullptr) frame = pager->select_victim_frame(frame_table);
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

// Page fault handler
bool pagefault_handler(process_object* process,
                        int vpage, 
                        pagerClass *pager, 
                        std::deque<int> &free_list,
                        frame_t* frame_table,
                        global_stats &gstats){

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

    frame_t* allocated_frame = get_frame(pager, free_list, frame_table);
    process->page_table[vpage].PHYSICAL_FRAME_NUMBER = allocated_frame->id;
    process->page_table[vpage].WRITE_PROTECT = vma_of_vpage->write_protected;
    process->page_table[vpage].PRESENT = 1;
    allocated_frame->mapped_pte = &(process->page_table[vpage]);
    allocated_frame->mapped_process = process;
    allocated_frame->mapped_vpage = vpage;
    allocated_frame->mapped_vma_id = vma_of_vpage->id;
    
    if (vma_of_vpage->file_mapped == true) {
        output(" FIN\n");
        process->pstats.fins++;
    }
    else if (process->page_table[vpage].PAGEDOUT) {
        output(" IN\n");
        process->pstats.ins++;
    }
    else {
        output(" ZERO\n");
        process->pstats.zeros++;
    }

    output(" MAP %d\n", allocated_frame->id);
    process->pstats.maps++;

    return true;
};


// ====================|  Simulation  |===========================


// Simulation
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


// ====================|  Main  |===========================
int main(int argc, char **argv) {
    int num_frames = MAX_FRAMES;
    int c;
    char algo = 'f';
    std::string input_file = "../lab3_assign/in1";
    std::string rfile = "rfile";

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
    else if (algo == 'r') {
        Randomizer randomizer(rfile);
        pager = new Random(num_frames, randomizer);
    }
    else if (algo == 'c') {
        pager = new Clock(num_frames);
    }
    else {
        std::cout << "Invalid algorithm" << std::endl;
        exit(1);
    }
    std::ifstream file(input_file);

    Randomizer randomizer(rfile);


    std::map<int, process_object> processes = readInput(file);
    printProcesses(processes);
    simulation(num_frames, processes, file, pager);
}