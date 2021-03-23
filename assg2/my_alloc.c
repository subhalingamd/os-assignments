#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#define _PAGE_SIZE 4096

typedef struct Node{
	size_t size;
	struct Node *next;
} Node;

Node *head = NULL;

void insert(void *addr, Node *head){
	Node *temp = head;
	head = (Node*) addr;
	head->size
}


////////////////////////////////////////////////////////////////

void* base = NULL;

void init_freelist(){
	head = (Node*)base;
	head->size=4096-8;
	head->next=NULL;
}


////////////////////////////////////////////////////////////////


int my_init(){
	base = mmap(NULL, _PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);

    if (base == MAP_FAILED) {
            return errno;
    }

    init_freelist();
	return 0;
}

void* my_alloc(int size){
	if (size%8 != 0) { return NULL; }
	// TODO
	return NULL;
}

void my_free(char *ptr){
	// TODO
	return;
}

void my_clean(){
	munmap(base, _PAGE_SIZE);
}

void my_heapinfo(){
	// TODO
	return;
}

int main(int argc, char *argv[]){

	int _errno = my_init();
	if (_errno){
		fprintf(stderr, "mmap failed: %s\n", strerror(_errno));
		exit(0);
	}
	my_clean();

	return 0;
}