#include <stdio.h>
#include <stdlib.h>

#include <string.h>
// #include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

// #define DEBUG // comment to turn off DEBUG mode

#define _PAGE_SIZE 4096
#define _RANDOM_SEED 5635  // as stated in assg specs
#define _HEX_SIZE 8 // does not include 0x
#define _RW_POS 12 // position of R/W in trace file
#define _MAX_TRACE_WC (10000000+1)  // upper bound for trace file size (used for OPT)

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))
#define max(X, Y) (((X) > (Y)) ? (X) : (Y))

typedef unsigned int _address;
#define get_PN(X) ((X & 0xfffff000) >> 12)

int NUM_FRAMES, VERBOSE;
char *TRACE_FILE;

#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

// int a=100, b=101, c=102, d=103;
void print_results(int a, int b, int c, int d){
    printf("Number of memory accesses: %d\n", a);
    printf("Number of misses: %d\n", b);
    printf("Number of writes: %d\n", c);
    printf("Number of drops: %d\n", d);
}

//For verbose:
//char read_page[] = "0x40345";
//char old_page[] = "0x40310";
//int replace_type = 0;
void print_verbose(int new_page, int old_page, int dirty_removal){
	if (VERBOSE == 0) { return; }

    if (dirty_removal == 1){
        printf("Page 0x%05x was read from disk, page 0x%05x was written to the disk.\n", new_page, old_page);
    }else{
        printf("Page 0x%05x was read from disk, page 0x%05x was dropped (it was not dirty).\n", new_page, old_page);
    }
}

void page_in_frames(int *dirty, char rw){
	if (rw == 'W'){
		*dirty = 1;
		// DEBUG_PRINT(("\tI: Setting dirty bit for pagenum 0x%05x\n WHEN FULL",pagenum));
	}
}

void replace_page(_address *frame, _address pagenum, int *dirty, char rw, int *reads, int *writes, int *drops){
	_address old_pagenum = *frame;
	*frame = pagenum;

	if (*dirty == 1) {
		++*writes;
		print_verbose(pagenum,old_pagenum,1);
	}
	else{
		++*drops;
		print_verbose(pagenum,old_pagenum,0);
	}

	if (rw == 'W'){
		*dirty = 1;
		DEBUG_PRINT(("\tI: Setting dirty bit for pagenum 0x%05x\n",pagenum));
	}
	else{
		*dirty = 0;
		DEBUG_PRINT(("\tI: Dropping dirty bit for pagenum 0x%05x\n",pagenum));
	}

	//DEBUG_PRINT(("dropping %d",idx));
	++*reads;
}

void add_page(_address *frame, _address pagenum, int *dirty, char rw, int *reads, int *count){
	*frame = pagenum;
	DEBUG_PRINT(("\tI: Adding new pagenum 0x%05x\n",pagenum));

	if (rw == 'W'){
		*dirty = 1;
	}
	else{
		*dirty = 0;
	}
	++*reads;
	++*count;
}




// Function to find the frame that will not be used in OPT
int predict_eviction_for_OPT(_address pages[], _address frames[], int num_pages, int curr_idx)
{
    // Store index of pages which will be used recently in future
    int res = -1, farthest = curr_idx;
    for (int i = 0; i < NUM_FRAMES; i++) {
        int j;
        for (j = curr_idx; j < num_pages; j++) {
            if (frames[i] == pages[j]) {
                if (j > farthest) {
                    farthest = j;
                    res = i;
                }
                break;
            }
        }
  
        // If a page is never referenced in future, return it.
        // DEBUG_PRINT(("\t\tI: %d\n",i));
        if (j == num_pages)
            return i;
    }
  
    // If all of the frames were not in future, return 0. 
    // Else we return 'res'
    return (res == -1) ? 0 : res;
}


void for_OPT(){

	int mem_access = 0, reads = 0, writes = 0, drops = 0;
	_address frames[NUM_FRAMES];

	int count = 0;
	int dirty[NUM_FRAMES];
	
	// this might not be required...
	for (int i = 0; i < NUM_FRAMES; i++) { dirty[i] = 0; }

	// _address trace_pns[_MAX_TRACE_WC]; char trace_rws[_MAX_TRACE_WC];
	_address *trace_pns = (_address *) malloc(_MAX_TRACE_WC*sizeof(_address));
	char *trace_rws = (char*) malloc(_MAX_TRACE_WC*sizeof(char));
	

    FILE* file = fopen(TRACE_FILE, "r"); /* should check the result */
    char line[64]; int trace_ctr=0;

    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
    	
    	if (!strcmp(line,"\n")) { continue; }

    	char virtadr[_HEX_SIZE+1]; int rw;
    	for (int i = 0; i < _HEX_SIZE; i++) {
    		virtadr[i] = line[i+2];
    	}
    	virtadr[_HEX_SIZE] = '\0';

    	_address pagenum = get_PN( (int)strtol(virtadr, NULL, 16)  );

    	DEBUG_PRINT((" - [%s -> 0x%05x - %c]\n",virtadr,pagenum,line[_RW_POS]));

    	trace_pns[trace_ctr] = pagenum;
    	trace_rws[trace_ctr] = line[_RW_POS];
    	trace_ctr++;
    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

    fclose(file);

    for (int tr_i = 0; tr_i < trace_ctr; tr_i++) {
    	DEBUG_PRINT(("\tI: 0x%05x \t %c\n",trace_pns[tr_i],trace_rws[tr_i]));
    	_address pagenum = trace_pns[tr_i];

    	// in both cases check if page is found...
    	if (count >= NUM_FRAMES){ // replace
    		int idx = -1; // note
    		for (int i=0; i<NUM_FRAMES; i++) {
    			if (pagenum == frames[i]){
    				idx = i;
    				break;
    			}
    		}

    		if (idx >=0 ){ 
    			page_in_frames(&dirty[idx],trace_rws[tr_i]);
    		}

    		else {
    			idx = predict_eviction_for_OPT(trace_pns, frames, trace_ctr, tr_i + 1);
    			
    			replace_page(&frames[idx], pagenum, &dirty[idx], trace_rws[tr_i], &reads, &writes, &drops);
    		}

    	}
    	else { // just insert
    		int idx = count; // note
    		for (int i=0; i<count; i++) {
    			if (pagenum == frames[i]){
    				idx = i;
    				break;
    			}
    		}


    		if (idx == count) { 
				add_page(&frames[idx], pagenum, &dirty[idx], trace_rws[tr_i], &reads, &count);    			
    		}
    		else { 
    			page_in_frames(&dirty[idx],trace_rws[tr_i]); 
    		}

    	}
    	mem_access++;
    	//break;

    	DEBUG_PRINT(("\tI: FRAMES:\t"));
    	for (int x=0; x<NUM_FRAMES; x++){
    		DEBUG_PRINT(("0x%05x  ",frames[x]));
    	}
    	DEBUG_PRINT(("\n\n"));

       	
    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

    print_results(mem_access,reads,writes,drops);

    //cleanup
    free(trace_pns); free(trace_rws);
	
}



// in CLOCK
	// what comes in -> 1 (f22)
		// For consistency, let's say that if the last replaced page was at position x∈{0,…,n−1} in the circular list, and then the algorithm circles back not being able to find any page with use bit 0, then next we evict (x+1)th frame
		// so jump to next index.. 
	// as u traverse ->change 1 to 0
	// when a loop completes, 0 is encountered agin -> so just drop
	//  the keeping count [not] req

void for_CLOCK(){

	int mem_access = 0, reads = 0, writes = 0, drops = 0;
	_address frames[NUM_FRAMES];

	int count = 0;
	int dirty[NUM_FRAMES];

	int timestamps[NUM_FRAMES], clock_hand = 0;
	
	// this might not be required...
	for (int i = 0; i < NUM_FRAMES; i++) { dirty[i] = 0; timestamps[i] = 0;}


    FILE* file = fopen(TRACE_FILE, "r"); /* should check the result */
    char line[64];

    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
    	
    	if (!strcmp(line,"\n")) { continue; }
    	char virtadr[_HEX_SIZE+1]; int rw;
    	for (int i = 0; i < _HEX_SIZE; i++) {
    		virtadr[i] = line[i+2];
    	}
    	virtadr[_HEX_SIZE] = '\0';

    	_address pagenum = get_PN( (int)strtol(virtadr, NULL, 16)  );

    	DEBUG_PRINT((" - [%s -> 0x%05x - %c]\n",virtadr,pagenum,line[_RW_POS]));

    	// in both cases check if page is found...
    	if (count >= NUM_FRAMES){ // replace
    		int idx = -1; // note
    		for (int i=0; i<NUM_FRAMES; i++) {
    			if (pagenum == frames[i]){
    				idx = i;
    				break;
    			}
    		}

    		if (idx >=0 ){ 
    			page_in_frames(&dirty[idx],line[_RW_POS]);
    			timestamps[idx] = 1;
    		}

    		else {
    			while (timestamps[clock_hand] != 0){
    				timestamps[clock_hand] = 0;
    				clock_hand = (clock_hand+1)%NUM_FRAMES;
    			}
    			idx = clock_hand;
    			
    			replace_page(&frames[idx], pagenum, &dirty[idx], line[_RW_POS], &reads, &writes, &drops);
    			timestamps[idx] = 1;

    			// One way to do it is to increase the pointer after replacing the page and making its use bit 1.
    			clock_hand = (clock_hand+1)%NUM_FRAMES;
    		}

    	}
    	else { // just insert
    		int idx = count; // note
    		for (int i=0; i<count; i++) {
    			if (pagenum == frames[i]){
    				idx = i;
    				break;
    			}
    		}


    		if (idx == count) { 
				add_page(&frames[idx], pagenum, &dirty[idx], line[_RW_POS], &reads, &count);
    			timestamps[idx] = 1;
    			
    		}
    		else { 
    			page_in_frames(&dirty[idx],line[_RW_POS]); 
    			timestamps[idx] = 1;
    		}

    	}
    	/// timestamps[idx] = mem_access;
    	mem_access++;
    	//break;

        //DEBUG_PRINT(("%s", line));
    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

    fclose(file);
    print_results(mem_access,reads,writes,drops);
	
}




void for_LRU(){

	int mem_access = 0, reads = 0, writes = 0, drops = 0;
	_address frames[NUM_FRAMES];

	int count = 0;
	int dirty[NUM_FRAMES];

	int timestamps[NUM_FRAMES];
	
	// this might not be required...
	for (int i = 0; i < NUM_FRAMES; i++) { dirty[i] = 0; timestamps[i] = 0;}
	

    FILE* file = fopen(TRACE_FILE, "r"); /* should check the result */
    char line[64];

    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
    	
    	if (!strcmp(line,"\n")) { continue; }
    	char virtadr[_HEX_SIZE+1]; int rw;
    	for (int i = 0; i < _HEX_SIZE; i++) {
    		virtadr[i] = line[i+2];
    	}
    	virtadr[_HEX_SIZE] = '\0';

    	_address pagenum = get_PN( (int)strtol(virtadr, NULL, 16)  );

    	DEBUG_PRINT((" - [%s -> 0x%05x - %c]\n",virtadr,pagenum,line[_RW_POS]));

    	// in both cases check if page is found...
    	if (count >= NUM_FRAMES){ // replace
    		int idx = -1; // note
    		for (int i=0; i<NUM_FRAMES; i++) {
    			if (pagenum == frames[i]){
    				idx = i;
    				break;
    			}
    		}

    		if (idx >=0 ){ 
    			page_in_frames(&dirty[idx],line[_RW_POS]);
    			timestamps[idx] = mem_access;
    		}

    		else {
    			idx = 0; int timest = timestamps[0];
    			for (int j=1; j<NUM_FRAMES; j++){
    				if (timestamps[j] < timest){
    					timest = timestamps[j];
    					idx = j;
    				}
    			}
    			
    			replace_page(&frames[idx], pagenum, &dirty[idx], line[_RW_POS], &reads, &writes, &drops);
    			timestamps[idx] = mem_access;
    		}

    	}
    	else { // just insert
    		int idx = count; // note
    		for (int i=0; i<count; i++) {
    			if (pagenum == frames[i]){
    				idx = i;
    				break;
    			}
    		}


    		if (idx == count) { 
				add_page(&frames[idx], pagenum, &dirty[idx], line[_RW_POS], &reads, &count);
    			timestamps[idx] = mem_access;
    			
    		}
    		else { 
    			page_in_frames(&dirty[idx],line[_RW_POS]); 
    			timestamps[idx] = mem_access;
    		}

    	}
    	/// timestamps[idx] = mem_access;
    	mem_access++;
    	//break;

        //DEBUG_PRINT(("%s", line));
    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

    fclose(file);
    print_results(mem_access,reads,writes,drops);
	
}



void for_RANDOM(){
	int mem_access = 0, reads = 0, writes = 0, drops = 0;
	_address frames[NUM_FRAMES];

	int count = 0;
	int dirty[NUM_FRAMES];
	for (int i = 0; i < NUM_FRAMES; i++) { dirty[i] = 0;}

	srand(_RANDOM_SEED);


    FILE* file = fopen(TRACE_FILE, "r"); /* should check the result */
    char line[64];

    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
    	
    	if (!strcmp(line,"\n")) { continue; }
    	char virtadr[_HEX_SIZE+1]; int rw;
    	for (int i = 0; i < _HEX_SIZE; i++) {
    		virtadr[i] = line[i+2];
    	}
    	virtadr[_HEX_SIZE] = '\0';

    	_address pagenum = get_PN( (int)strtol(virtadr, NULL, 16)  );

    	DEBUG_PRINT((" - [%s -> 0x%05x - %c]\n",virtadr,pagenum,line[_RW_POS]));

    	// in both cases check if page is found...
    	if (count >= NUM_FRAMES){ // replace
    		int idx = -1; // note
    		for (int i=0; i<NUM_FRAMES; i++) {
    			if (pagenum == frames[i]){
    				idx = i;
    				break;
    			}
    		}

    		if (idx >=0 ){ page_in_frames(&dirty[idx],line[_RW_POS]); }

    		else {
    			idx = rand() % NUM_FRAMES; // idx will be evicted
    			
    			replace_page(&frames[idx], pagenum, &dirty[idx], line[_RW_POS], &reads, &writes, &drops);
    			
    		}

    	}
    	else { // just insert
    		int idx = count; // note
    		for (int i=0; i<count; i++) {
    			if (pagenum == frames[i]){
    				idx = i;
    				break;
    			}
    		}


    		if (idx == count) { 
				add_page(&frames[idx], pagenum, &dirty[idx], line[_RW_POS], &reads, &count);
    			
    			
    		}
    		else { page_in_frames(&dirty[idx],line[_RW_POS]); }

    	}

    	mem_access++;
    	//break;

        //DEBUG_PRINT(("%s", line));
    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

    fclose(file);
    print_results(mem_access,reads,writes,drops);
	
}

void for_FIFO(){

	int mem_access = 0, reads = 0, writes = 0, drops = 0;
	_address frames[NUM_FRAMES];

	int count = 0;
	int dirty[NUM_FRAMES];
	for (int i = 0; i < NUM_FRAMES; i++) { dirty[i] = 0;}


	int head = -1;
	
	///// ++start; start%=NUM_FRAMES; // if count<MAX_HIST, start<MAX_HIST
	// TODO :: strcpy(history[*start],inp); // copy each char in inp to history
	
	///// if (count < NUM_FRAMES) count++;
	//(*tot_count)++;


    FILE* file = fopen(TRACE_FILE, "r"); /* should check the result */
    char line[64];

    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
    	
    	if (!strcmp(line,"\n")) { continue; }
    	char virtadr[_HEX_SIZE+1]; int rw;
    	for (int i = 0; i < _HEX_SIZE; i++) {
    		virtadr[i] = line[i+2];
    	}
    	virtadr[_HEX_SIZE] = '\0';

    	_address pagenum = get_PN( (int)strtol(virtadr, NULL, 16)  );

    	DEBUG_PRINT((" - [%s -> 0x%05x - %c]\n",virtadr,pagenum,line[_RW_POS]));

    	// in both cases check if page is found...
    	if (count >= NUM_FRAMES){ // replace
    		int idx = -1; // note
    		for (int i=0; i<NUM_FRAMES; i++) {
    			if (pagenum == frames[i]){
    				idx = i;
    				break;
    			}
    		}

    		if (idx >=0 ){ page_in_frames(&dirty[idx],line[_RW_POS]); }

    		else {
    			idx = ++head % NUM_FRAMES; // idx will be evicted
    			
    			replace_page(&frames[idx], pagenum, &dirty[idx], line[_RW_POS], &reads, &writes, &drops);
    			
    		}

    	}
    	else { // just insert
    		int idx = count; // note
    		for (int i=0; i<count; i++) {
    			if (pagenum == frames[i]){
    				idx = i;
    				break;
    			}
    		}


    		if (idx == count) { 
				add_page(&frames[idx], pagenum, &dirty[idx], line[_RW_POS], &reads, &count);
    			
    			
    		}
    		else { page_in_frames(&dirty[idx],line[_RW_POS]); }

    	}

    	mem_access++;
    	//break;

        //DEBUG_PRINT(("%s", line));
    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

    fclose(file);
    print_results(mem_access,reads,writes,drops);
	
}


int main(int argc, char *argv[]){
	
	//assert(argc == 3 || argc == 4);
	if (!(argc == 3 +1 || argc == 4 +1)) { fprintf(stderr, "Arguments missing.\n"); exit(0); }
	TRACE_FILE = argv[1]; NUM_FRAMES = atoi(argv[2]); VERBOSE = (argc==5 && !strcmp(argv[4],"-verbose"));
	
	/*
	_address a = 0x41233af3;
	DEBUG_PRINT(("0x%05x --> 0x%05x\n",a,get_PN(a)));
	*/

	// Check if trace file exists
	FILE* file = fopen(TRACE_FILE, "r"); /* should check the result */
	if (file == NULL) {
		fprintf(stderr, "Trace file not found.\n");
		exit(0);
	}
	fclose(file);

	DEBUG_PRINT(("\t[%s,%d,%d]\n",TRACE_FILE,NUM_FRAMES,VERBOSE));

	// OPT, FIFO, CLOCK, LRU, and RANDOM.
	if (!strcmp(argv[3],"OPT")){
		DEBUG_PRINT(("I: Calling OPT...\n"));
		for_OPT();
	}
	else if (!strcmp(argv[3],"FIFO")){
		DEBUG_PRINT(("I: Calling FIFO...\n"));
		for_FIFO();
	}
	else if (!strcmp(argv[3],"CLOCK")){
		DEBUG_PRINT(("I: Calling CLOCK...\n"));
		for_CLOCK();
	}
	else if (!strcmp(argv[3],"LRU")){
		DEBUG_PRINT(("I: Calling LRU...\n"));
		for_LRU();
	}
	else if (!strcmp(argv[3],"RANDOM")){
		DEBUG_PRINT(("I: Calling RANDOM...\n"));
		for_RANDOM();
	}
	else{
		fprintf(stderr, "Something went wrong. Method not found.\n\tProTip: Choose from [OPT, FIFO, CLOCK, LRU, RANDOM]\n");
	}

	return 0;
}