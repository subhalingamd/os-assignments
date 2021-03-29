#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
#include <errno.h>

#define _PAGE_SIZE 4096
#define _MAGIC_NUM 45816

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))
#define max(X, Y) (((X) > (Y)) ? (X) : (Y))

typedef struct Node{
	int size;
	struct Node *next;
} Node;


////////////////////////////////////////////////////////////////

typedef struct _header{
	int size;
	int magic;
} _header;


////////////////////////////////////////////////////////////////

typedef struct _heap_info{
	Node *head;
	int small_chunk;
	int large_chunk;
	int blocks_count;
	int curr_size;
} _heap_info;


////////////////////////////////////////////////////////////////
void *base = NULL;
/////////////////////////////////////////////////////////////////

void init_heap_info(){
	_heap_info *det = (_heap_info*)(base);
	det->large_chunk = det->small_chunk = _PAGE_SIZE - sizeof(_heap_info) - sizeof(Node);
	det->blocks_count = 0;
	det->curr_size = sizeof(Node);
	// check vals once...

	
	//printf("Adding _heap_info at %d\n",(int)(base));
	/*
	printf("large chunk %d\n",((_heap_info*)(base))->large_chunk);
	printf("blocks_count %d\n",((_heap_info*)(base))->blocks_count);
	printf("curr_size %d\n",((_heap_info*)(base))->curr_size);
	printf("First node at %p\n",(Node*)((_heap_info*)(base)+1));
	*/
	/*
	printf("Address of base: %d\n",(int)base);
	printf("Address of ->head: %d\n",(int)&det->head);
	printf("Address of ->small_chunk: %d\n",(int)&det->small_chunk);
	printf("Address of ->large_chunk: %d\n",(int)&det->large_chunk);
	printf("Address of ->blocks_count: %d\n",(int)&det->blocks_count);
	printf("Address of ->curr_size: %d\n",(int)&det->curr_size);
	*/
}

void init_freelist(){
	Node *head = (Node*)(((_heap_info*)base)+1);
	head->size= _PAGE_SIZE - sizeof(Node) - sizeof(_heap_info);
	head->next=NULL;

	((_heap_info*)base)->head = head;

	
	// printf("Currently head at %d\n",(int)head);
	// printf("head in _heap_info : %d\n\n",((_heap_info*)base)->head);
	

	/* printf("Address of head: %d\n",(int)head); */

	/*
	printf("head->size through pointer: %d\n",*((int*)(((_heap_info*)base)->head)));
	printf("head->size actual val: %d\n",(int)((Node*)(((_heap_info*)base)->head))->size);
	printf("head->size address: %p\n",&((Node*)(((_heap_info*)base)->head))->size);
	printf("head->next address: %p\n",&((Node*)(((_heap_info*)base)->head))->next);
	*/
}

// deprecated function
Node* find_suitable_block(int size){
	/*
	Node *t = head, *res = NULL;
	while (t!=NULL){
		if (size <= t->size && (res==NULL || res->size < t->size)){
			res = t;
		}
		t = t->next;
	}
	return res;
	*/
	return NULL;
}

// deprecated function
void split(Node* nd, int size){
	/*_header *h = (_header*) nd;
	if (size == nd->size - sizeof(Node) + sizeof(_header)){
		//TODO
		return;
	}
	// what if node is head?
	Node *t = (Node*) (nd+size+sizeof(_header));
	t->size=nd->size; t->next=nd->next;
	
	h->size = size + sizeof(_header);
	printf("<<<%p>>>\n", t);
	//printf("<<<%lu>>>\n", sizeof(_header*));
	//printf("<<<%lu>>>\n", sizeof(Node*));

*/
	return;

}



////////////////////////////////////////////////////////////////


int my_init(){
	base = mmap(NULL, _PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);

    if (base == MAP_FAILED) {
            return errno;
    }
    init_heap_info();
    init_freelist();
	return 0;
}

void* my_alloc(int size){

	if (size%8 != 0) { return NULL; }
	if (size <= 0) { return NULL; }
	// TODO

	Node *head = (Node*)(((_heap_info*)base)->head);
	// printf("myalloc says head is %d\n",(int)head);

	// no free space / one node / 2+ nodes...
	if (head == NULL) { // no free space
		// printf("head==NULL>>>>\n\n");
		_heap_info *det = (_heap_info*)(base);
		det->large_chunk = det->small_chunk = 0 ;
		return NULL;
	}

	if (head->next == NULL) { // only single node
		// printf("Single node in free list....\n");
		if (size + sizeof(_header) > head->size + sizeof(Node)){ // no space
			// printf("---Overflow...\n");
			_heap_info *det = (_heap_info*)(base);
			det->large_chunk = det->small_chunk = head->size;
			return NULL;
		}
		else{
			if (size + sizeof(_header) <= head->size){ // size rem >= Node size
				Node *new_head = ((void*)(head)+size+sizeof(_header));
				// printf("---Node retained---\n");
				// printf("[Changing from %d to %d]\n",(int)head,(int)new_head);
				
				new_head->size = head->size - size - sizeof(_header);
				new_head->next = NULL;

				((_heap_info*)base)->head = new_head;

				_header *alloc = (_header*)head;
				alloc->size = size; alloc->magic = _MAGIC_NUM;
				
				/*
				printf("[Now, old_head->size will be size in header = %d]\n",(int)*(int*)head);
				printf("[Alloc at %d]\n",(int)alloc);
				printf("[New head is at %d (%d)]\n\n",(int)((_heap_info*)base)->head,(int)new_head);
				*/
				_heap_info *det = (_heap_info*)(base);
				det->large_chunk = det->small_chunk = new_head->size;
				det->curr_size += (size + sizeof(_header));
				det->blocks_count += 1; 

				return ((void*)(alloc)+sizeof(_header));
			}
			else{
				// printf("---Node removed---\n");
				//printf("[Changing size from %d",size);
				size = head->size + (int)sizeof(Node) - sizeof(_header);
				//printf("to %d]\n",size);

				((_heap_info*)base)->head = NULL;

				_header *alloc = (_header*)head;
				alloc->size = size; alloc->magic = _MAGIC_NUM;

				_heap_info *det = (_heap_info*)(base);
				det->large_chunk = det->small_chunk = 0;
				det->curr_size += (size + sizeof(_header) - sizeof(Node)); // = head->size
				det->blocks_count += 1; 
				//printf("[Alloc at %d]\n",(int)alloc);
				return ((void*)(alloc)+sizeof(_header));
			}
		}
	}
	else{ // at least two nodes in free list...
		int a=head->size, b=head->next->size;
		int largest, smallest, second_largest;
		Node *t = head->next->next, *t_prev = head->next, *res = NULL, *prev = NULL;

		if (a<b){
			largest = b;
			smallest = a; second_largest = a;
			if ( size+sizeof(_header) <= b + sizeof(Node) ){
				res = head->next;
				prev = head;
			}
		}
		else {
			largest = a;
			smallest = b; second_largest = b;
			if ( size+sizeof(_header) <= a + sizeof(Node) ){
				res = head;
				prev= NULL;
			}
		}

		while (t!=NULL){
			if (t->size>largest) {
				second_largest=largest; largest=t->size;
			}
			else if (t->size>second_largest) {second_largest=t->size; }

			if (t->size<smallest) {smallest=t->size;}

			if (size + sizeof(_header) <= t->size + sizeof(Node) && (res==NULL || res->size < t->size)){
				res = t;
				prev = t_prev;
			}
			t_prev = t;
			t = t->next;
		}
		
		/*
		printf("More than one node in free list....\n");
		printf("- [L:%d; 2L:%d; S:%d]\n;",largest,second_largest,smallest);
		*/

		if (res==NULL){ // no space
			// printf("---Overflow...\n");
			_heap_info *det = (_heap_info*)(base);
			det->large_chunk = largest;
			det->small_chunk = smallest;
			return NULL;
		}
		else{
			if (size + sizeof(_header) <= res->size){ // size rem >= Node size
				Node *new_head = ((void*)(res)+size+sizeof(_header));
				// printf("---Node retained---\n");
				//printf("[Changing from %d to %d]\n",(int)res,(int)((void*)(res)+size+sizeof(_header)));
				
				new_head->size = res->size - size - sizeof(_header);
				new_head->next = res->next;

				if (prev == NULL){ // head
					((_heap_info*)base)->head = new_head;
				}
				else{
					prev->next = new_head;
				}

				_header *alloc = (_header*)res;
				alloc->size = size; alloc->magic = _MAGIC_NUM;
				
				
				//printf("[Now, old_head->size will be size in header = %d]\n",(int)*(int*)res);
				//printf("[Returns %d]\n",(int)((void*)(alloc)+sizeof(_header)));
				
				_heap_info *det = (_heap_info*)(base);
				
				int largest_updated = largest-size-sizeof(_header);
				det->large_chunk = max(largest_updated,second_largest);
				
				/*
				if (largest_updated>0) {det->small_chunk = min(largest_updated,smallest);}
				else {det->small_chunk = smallest;}
				*/
				// 0 is allowed as chunk exists!
				det->small_chunk = min(largest_updated,smallest);

				det->curr_size += (size + sizeof(_header));
				det->blocks_count += 1; 

				return ((void*)(alloc)+sizeof(_header));
			}
			else{
				// printf("---Node removed---\n");
				//printf("[Changing size from %d",size);
				size = res->size + (int)sizeof(Node) - (int)sizeof(_header);
				//printf("to %d]\n",size);

				// has to come before alloc (for 32-bits systems)
				if (prev == NULL){ // head
					((_heap_info*)base)->head = res->next;
				}
				else{
					prev->next = res->next;
				}

				_header *alloc = (_header*)res;
				alloc->size = size; alloc->magic = _MAGIC_NUM;

				// the largest chunk doesn't exists anymore... :/
				_heap_info *det = (_heap_info*)(base);
				det->large_chunk = second_largest;
				det->small_chunk = smallest;
				det->curr_size += (size + sizeof(_header) - sizeof(Node)); // = res->size
				det->blocks_count += 1; 

				return ((void*)(alloc)+sizeof(_header));
			}
		}

		// should not reach here...
		return NULL;
	}

	

	/*
	Node *nd = find_suitable_block(size);
	if (nd == NULL)	{ return NULL; }
	
	split(nd,size);
	return nd;
	*/


}

void my_free(void *ptr){
	// TODO
	//check if free is valid..  also if null, lies within bounds, sanity
	if (ptr==NULL) {return;}

	_header *h = ((_header*)ptr)-1;
	//printf("--[%d(data start) goes to %d(header start)]\n",(int)ptr,(int)h);

	int size = h->size;
	Node *guest = (Node*)h;
	guest->size = size + sizeof(_header) - sizeof(Node);
	guest->next = NULL;
	//printf("--[Guest added at %d]\n",(int)guest);
	//printf("--[Guest->size: %d]\n",(int)guest->size);

	((_heap_info*)(base))->blocks_count -= 1;

	// Node *head = ((_heap_info*)base)->head; .. not req
	Node *guest_top=NULL, /* *guest_top_prev=NULL ,*/ *guest_bottom=NULL,*guest_bottom_prev=NULL;
	Node *cur= ((_heap_info*)base)->head, *prev= NULL;

	// if head --> NULL => cur -- > NULL -- so doesn't enter loop
	while (cur != NULL){
		if ((void*)(cur) + cur->size + sizeof(Node) == guest){ // top
			//printf("******Top neighbour Found******\n");
			guest_top = cur;
			// guest_top_prev = prev; // not reqd
		}
		else if ((void*)(guest) + guest->size + sizeof(Node) == cur){ // bottom // mutually excl
			//printf("******Bottom neighbour Found******\n");
			guest_bottom = cur;
			guest_bottom_prev = prev;
		}
		prev = cur;
		cur = cur->next;
	}

	// four cases
		// if head --> NULL => since no loop => guest_top == guest_bottom == NULL
	if (guest_top == NULL && guest_bottom == NULL){ // no neighbours
		guest->next = ((_heap_info*)base)->head;
		((_heap_info*)base)->head = guest;

		// if small chunk == 0 && large chunk == 0 => we cant determine if chunk exists and that size is 0 or chunk doesn't exist itself...
		//if ( (((_heap_info*)(base))->small_chunk == 0) || (((_heap_info*)(base))->large_chunk==-1 && ((_heap_info*)(base))->small_chunk == -1) ){
		if ( (((_heap_info*)(base))->large_chunk <= 0 && ((_heap_info*)(base))->small_chunk <= 0) ){
			// invalidate
			((_heap_info*)(base))->large_chunk = -1;
			((_heap_info*)(base))->small_chunk = -1;
		}
		else{
			/*
			int bs = ((_heap_info*)(base))->large_chunk;
			((_heap_info*)(base))->large_chunk = max(bs,guest->size);

			bs = ((_heap_info*)(base))->small_chunk;
			//if (bs > 0) { ((_heap_info*)(base))->small_chunk = min(bs,guest->size); }
			//else { ((_heap_info*)(base))->small_chunk = guest->size;}
			((_heap_info*)(base))->small_chunk = min(bs,guest->size);
			*/

			if (guest->size > ((_heap_info*)(base))->large_chunk){
				((_heap_info*)(base))->large_chunk = guest->size;
			}
			if (guest->size < ((_heap_info*)(base))->small_chunk){
				((_heap_info*)(base))->small_chunk = guest->size;
			}

		}
		
		
		((_heap_info*)(base))->curr_size -= (guest->size);
	}

	else if (guest_bottom == NULL ){ /// top exists but not bottom
		//printf("** guest_top->size changes from %d",guest_top->size);
		guest_top->size += (guest->size + sizeof(Node));
		//printf("to %d ** \n",guest_top->size);

		// invalidate
		((_heap_info*)(base))->large_chunk = -1;
		((_heap_info*)(base))->small_chunk = -1;
		((_heap_info*)(base))->curr_size -= (guest->size + sizeof(Node));
	} 
	else if (guest_top == NULL) { // bottom exists but not top
		if (guest_bottom_prev == NULL){ // head
			((_heap_info*)base)->head = guest;
		}
		else{
			guest_bottom_prev->next = guest;
		}

		((_heap_info*)(base))->curr_size -= (guest->size + sizeof(Node));

		guest->size += (guest_bottom->size + sizeof(Node));
		guest->next = guest_bottom->next;

		// invalidate
		((_heap_info*)(base))->large_chunk = -1;
		((_heap_info*)(base))->small_chunk = -1;
	}

	else{ // both top and bottom exists
		guest_top->size += (guest->size + guest_bottom->size + 2*sizeof(Node));
		if (guest_bottom_prev == NULL){ // head
			((_heap_info*)base)->head = guest_bottom->next;
		}
		else{
			guest_bottom_prev->next = guest_bottom->next;
		}
		// invalidate
		((_heap_info*)(base))->large_chunk = -1;
		((_heap_info*)(base))->small_chunk = -1;
		((_heap_info*)(base))->curr_size -= (guest->size + 2*sizeof(Node));
	}


	return;
}

void my_clean(){
	munmap(base, _PAGE_SIZE);
}

void my_heapinfo(){
	// TODO
	int s=((_heap_info*)base)->small_chunk, l=((_heap_info*)base)->large_chunk;

	if (s==-1 && l==-1){
		Node *t = ((_heap_info*)base)->head;
		if (t==NULL) { s=l=0; } // this might not be reqd...
		else{
			s=l=t->size;
			t=t->next;
			while (t!=NULL){
				/* This is C... sad :/
				s = min(s, t->size);
				l = min(l, t->size);
				*/
				if (t->size < s){
					s = t->size;
				}
				else if (t->size > l){
					l = t->size;
				}
				t=t->next;
			}

		}
	}

	((_heap_info*)base)->small_chunk = s; ((_heap_info*)base)->large_chunk = l;

	// Do not edit below output format
	printf("=== Heap Info ================\n");
	printf("Max Size: %d\n", _PAGE_SIZE- (int)sizeof(_heap_info));
	printf("Current Size: %d\n", ((_heap_info*)base)->curr_size);
	printf("Free Memory: %d\n", _PAGE_SIZE- (int)sizeof(_heap_info) - ((_heap_info*)base)->curr_size);
	printf("Blocks allocated: %d\n", ((_heap_info*)base)->blocks_count);
	printf("Smallest available chunk: %d\n", s);
	printf("Largest available chunk: %d\n", l);
	printf("==============================\n");
	// Do not edit above output format
	return;
}

/*
void invalidate1(){
	((_heap_info*)(base))->large_chunk = -1;
	((_heap_info*)(base))->small_chunk = -1;
	my_heapinfo();
}

void traverse1(){
	Node *head = (Node*)(((_heap_info*)base)->head);
	printf("\n\n\n");
	while (head!=NULL){
		printf("-->%d",head->size);
		head = head->next;
	}
	printf("||\n\n\n");
}
*/


/****************************************************************
int main(int argc, char *argv[]){
	
	int _errno = my_init();
	if (_errno){
		fprintf(stderr, "mmap failed: %s\n", strerror(_errno));
		exit(0);
	}
****************************************************************/

	/*
	printf("%p\n",base);
	printf("sizeof(Node): %lu\n",sizeof(Node));
	printf("sizeof(Node*): %lu\n",sizeof(Node*));
	printf("sizeof(int): %lu\n",sizeof(int));
	printf("sizeof(size_t): %lu\n",sizeof(size_t));
	printf("sizeof(_header): %lu\n",sizeof(_header));
	printf("sizeof(_header*): %lu\n",sizeof(_header*));
	*/
	
	/*
	printf("Head: %p\n",*(Node*)(base));
	printf("Head+1: %p\n",(Node*)(base)+1);
	*/
	/*
	// T1 starts
	void* ptr1 = my_alloc(400);
	printf("|||>>>%d|||\n",(int)ptr1);
	my_heapinfo();
	void* ptr2 = my_alloc(96);
	printf("|||>>>%d|||\n",(int)ptr2);
	my_heapinfo();
	//void* ptr3 = my_alloc(64);
	//printf("|||>>>%d|||\n",(int)ptr3);
	//my_heapinfo();
	void* ptr4 = my_alloc(96);
	printf("|||>>>%d|||\n",(int)ptr4);
	my_heapinfo();
	printf("\n\n\nMyalloc passed\n");
	my_free(ptr1);
	my_heapinfo();
	ptr1 = my_alloc(24);
	printf("|||>>>%d|||\n",(int)ptr1);
	my_heapinfo();
	void* ptr5 = my_alloc(96);
	printf("|||>>>%d|||\n",(int)ptr5);
	my_heapinfo();
	void* ptr6 = my_alloc(96);
	printf("|||>>>%d|||\n",(int)ptr6);
	my_heapinfo();
	void *ptr7 = my_alloc(136);
	printf("|||>>>%d|||\n",(int)ptr7);
	my_heapinfo();
	my_free(ptr6);
	my_heapinfo();
	T1 ends*/


	/*
	// T2 starts  ... doesn't clear the whole free list
	my_heapinfo();
	void *ptr1 = my_alloc(48);
	my_heapinfo();
	void *ptr2 = my_alloc(96);
	my_heapinfo();
	void *ptr3 = my_alloc(160);
	my_heapinfo();
	void *ptr4 = my_alloc(40);
	my_heapinfo();
	void *ptr5 = my_alloc(88);
	my_heapinfo();
	void *ptr6 = my_alloc(88); // can't allocate
	my_heapinfo();
	my_free(ptr1); // single block free
	my_heapinfo();
	my_free(ptr2); // top exists
	my_heapinfo();
	my_free(ptr5); // bottom exists 
	my_heapinfo();
	my_free(ptr4); // bottom exists
	my_heapinfo();
	my_free(ptr3); // both top and bottom exists
	my_heapinfo();

	ptr1 = my_alloc(480); // full -> head=NULL
	my_heapinfo();
	my_free(ptr1);
	ptr1 = my_alloc(472); // partially full -> make it 488
	my_heapinfo();
	//T2 end*/

	/*
	// T3
	my_heapinfo();
	void *ptr1 = my_alloc(56);
	my_heapinfo();
	void *ptr2 = my_alloc(96);
	my_heapinfo();
	void *ptr3 = my_alloc(160);
	my_heapinfo();
	void *ptr4 = my_alloc(32);
	my_heapinfo();
	void *ptr5 = my_alloc(88);
	my_heapinfo();
	void *ptr6 = my_alloc(88); // can't allocate
	my_heapinfo();
	my_free(ptr1); // single block free
	my_heapinfo();
	my_free(ptr4); // single block free
	my_heapinfo();
	ptr1 = my_alloc(24); //alloc in chunk 1
	my_heapinfo();
	my_free(ptr5); // top&bottom exists 
	my_heapinfo();
	ptr4 = my_alloc(32); //alloc in chunk 4-5
	my_heapinfo();





	my_free(ptr2); // top exists
	my_heapinfo();
	
	my_heapinfo();
	my_free(ptr3); // both top and bottom exists
	my_heapinfo();

	*/


	/*
	//T start -- for === Max Size: 488
	my_heapinfo();
	void *ptr0 = my_alloc(8000);  // single node overflow
	my_heapinfo();
	void *ptr1 = my_alloc(8);	// single node enough free space
	//printf("returned %d]************\n",(int)ptr1);
	my_heapinfo();
	void *ptr2 = my_alloc(8);
	my_heapinfo();
	void *ptr3 = my_alloc(16);
	my_heapinfo();
	void *ptr4 = my_alloc(16);
	my_heapinfo();
	void *ptr5 = my_alloc(16);
	my_heapinfo();
	void *ptr6 = my_alloc(16);
	my_heapinfo();
	void *ptr7 = my_alloc(16);
	my_heapinfo();
	void *ptr8 = my_alloc(16);
	my_heapinfo();
	void *ptr9 = my_alloc(16);
	my_heapinfo();
	void *ptr10 = my_alloc(16);
	my_heapinfo();
	void *ptr11 = my_alloc(16);
	my_heapinfo();
	void *ptr12 = my_alloc(16);
	my_heapinfo();
	void *ptr13 = my_alloc(16);
	my_heapinfo();
	void *ptr14 = my_alloc(16);
	my_heapinfo();
	void *ptr15 = my_alloc(16);
	my_heapinfo();
	void *ptr16 = my_alloc(16);
	my_heapinfo();
	void *ptr17 = my_alloc(16);
	my_heapinfo();
	void *ptr18 = my_alloc(16);
	my_heapinfo();
	void *ptr19 = my_alloc(16);
	my_heapinfo();
	void *ptr20 = my_alloc(16);
	my_heapinfo();
	void *ptr21 = my_alloc(8); // single node -- remove the node from free list
	//printf("returned %d]************\n",(int)ptr21);
	my_heapinfo();
	void *ptr22 = my_alloc(8); // no free space (head-->NULL)
	my_heapinfo();
	traverse1();
	my_free(ptr13);		// single block added
	my_heapinfo();
	my_free(ptr12); 	// bottom neighbour
	// my_heapinfo();
	my_free(ptr10);
	my_heapinfo();
	traverse1();
	my_free(ptr8);
	my_free(ptr6);
	traverse1();
	my_free(ptr11);		// top & bottom neighbour
	my_heapinfo();
	traverse1();

	my_free(ptr5);
	my_heapinfo();
	traverse1();
	void *ptr23 = my_alloc(8); // enough space--largest remains largest
	//printf("returned %d]************\n",(int)ptr23);
	my_heapinfo();
	traverse1();

	void *ptr24 = my_alloc(40); // enough space-- second largest becomes largest
	my_heapinfo();
	//printf("returned %d]************\n",(int)ptr24);
	traverse1();
	void *ptr25 = my_alloc(4000); // multinode overflow
	my_heapinfo();
	traverse1();
	void *ptr26 = my_alloc(40); // remove the node -- second largest is largest and smallest is smallest
	//printf("returned %d]************\n",(int)ptr26);
	my_heapinfo();
	traverse1();
	void *ptr27 = my_alloc(8); // enough space -- second largest is largest and smallest is 0
	my_heapinfo();
	traverse1();
	my_free(ptr1);
	my_free(ptr3);
	my_free(ptr16);
	my_free(ptr18);
	my_free(ptr20);
	my_heapinfo();
	traverse1();

	my_free(ptr17);
	my_heapinfo();
	traverse1();

	void *ptr28 = my_alloc(8);
	my_heapinfo();
	traverse1();

	void *ptr29 = my_alloc(8);
	my_heapinfo();
	traverse1();

	void *ptr30 = my_alloc(8);
	my_heapinfo();
	traverse1();

	void *ptr31 = my_alloc(16);
	my_heapinfo();
	traverse1();

	void *ptr32 = my_alloc(16);
	my_heapinfo();
	traverse1();

	my_alloc(8);
	my_alloc(8);
	my_alloc(8);
	my_alloc(8);
	my_alloc(8);
	my_alloc(8);
	my_alloc(8);
	my_heapinfo();
	traverse1();

	//T ends
	*/


	/*
	void *ptr = my_alloc(8);
	/// printf("///%p\n",ptr);
	if (ptr == NULL) {printf("null");}
	else {printf("<%p>",ptr);}
	
	my_heapinfo();
	*/

/****************************************************************
	my_clean();

	return 0;
}
****************************************************************/