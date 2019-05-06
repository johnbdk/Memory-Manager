#include "memory_management.h"

void mem_init(){

	printf("INIT \n");
	for (int i = 0; i < 9; ++i) {
		my_mem[i].address = NULL;
		my_mem[i].next_address = 0;
		my_mem[i].free_list = queue_create();
	}
}

int pos_of_object(size_t size){					// returns the position in array that the objects of a specific size should be placed
	int position;								// size 8 -> position 0, size 16 -> position 1, ...

	position = (int) (log(max(size,8))/log(2));
	position -= 3;
	//printf("size = %d: position = %d\n", size, position);
	return position;
}

int object_size_exists(size_t size){			// if objects of this size already exist (there is already a slot in the array)
	int position;

	position = pos_of_object(size);

	if(my_mem[position].address == NULL){
		return -1;
	}

	return position;
}

void mem_allocate(size_t size){	
	int position = pos_of_object(size);

	my_mem[position].address = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);			// allocate 1 page for each object size
}

void *my_malloc(size_t size){									// function used by users
	size_t object_size;
	void *address;
	int position;

	object_size = (int) pow(2, ceil(log(size)/log(2)) );
	printf("size of alloc: %zu, next pow of 2: %zu\n", size, object_size);

	position = object_size_exists(object_size);
	if( position == -1 ){
		mem_allocate(object_size);
		position = pos_of_object(object_size);
	}
	address = dequeue_head_no_lock(my_mem[position].free_list);
	if( address != NULL ){					// get from superblock
		printf("GOT FROM FREE LIST\n");
		return address;
	}
	else if( (((long int) my_mem[position].address) + 4096) > my_mem[position].next_address ){							// get from free list
		printf("GOT FROM SUPERBLOCK\n");
		address = (void *) (((long int) my_mem[position].address) + my_mem[position].next_address);
		my_mem[position].next_address += object_size;
	}

				// MUST CHECK IF THERE IS NO ITEM IN FREE LIST AND NO OTHER MEMORY IN SUPERBLOCK

	return address;
}

int my_free(void *address){
	long int mask;
	void *temp_addr;

	mask = ~(PAGE_SIZE - 1);
	temp_addr = (void *) (mask & ((long int) address));

	int i = 0;
	for (; i < SLOTS; i++) {
		if( temp_addr == my_mem[i].address){										// find in what free list to insert freed memory
			enqueue_head_no_lock(my_mem[i].free_list, address);
			printf("inserted to free %p\n", address);
			break;
		}
	}

	int numOf8B = pow(2, i+3)/8;
	const char helpBuf[8] = {0, 0, 0, 0, 0, 0, 0, '\0'};
	for (int j = 0; j <= numOf8B; j++) {
		strncpy((char*)address, helpBuf, 8);
		address += 8;
	}

	//printf("my_free:\n\tslot = %d, numOf8B = %d\n", i, numOf8B);
	return 0;
}
