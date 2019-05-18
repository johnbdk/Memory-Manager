#include "streamflow.h"


local_heap_t mem = {{{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
					{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}}};


int get_object_class(size_t size){					// returns the position in array that the objects of a specific size should be placed
	int position;								// size 8 -> position 0, size 16 -> position 1, ...

	position = (int) (log(max(size,8))/log(2)); // todo change
	position -= 3;
	//printf("size = %d: position = %d\n", size, position);
	return position;
}

int object_class_exists(size_t size){			// if objects of this size already exist (there is already a slot in the array)
	int position;

	position = get_object_class(size);

	if(mem.obj[position].active_head == NULL){
		return -1;
	}

	return position;
}

void allocate_memory(size_t size){	
	int position = get_object_class(size);
	pageblock_t *new_pageblock;
	void *temp_addr;
	unsigned long mask = 15;
	int allocate_size = PAGEBLOCK_SIZE;			// this will be replaced with more clever logic

	unsigned long page_block_mask = ~(allocate_size - 1);


	temp_addr = mmap(NULL, 2*allocate_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);			// allocate 1 page for each object size
	new_pageblock = (pageblock_t *) ( (((unsigned long)temp_addr) & page_block_mask)+ allocate_size );

	unsigned long temp_size =  ((unsigned long)new_pageblock) - ((unsigned long)temp_addr) ;
	munmap(temp_addr, temp_size );
	temp_size =  (((unsigned long)temp_addr) + 2*allocate_size) - ( ((unsigned long)new_pageblock) + allocate_size ) ;
	munmap( (void *) ( ((unsigned long)new_pageblock) + allocate_size ) , temp_size );


	new_pageblock->id = 0;
	new_pageblock->pageblock_size = allocate_size;
	new_pageblock->object_size = size;

	new_pageblock->unallocated = (void *) (( ((unsigned long) new_pageblock) + sizeof(pageblock_t) + mask ) & (~mask) );
	printf("~~~~~~ new_pageblock: %p\n", new_pageblock);

	new_pageblock->num_free_objects =  (unsigned) ((((unsigned long) new_pageblock) + allocate_size) - ((unsigned long) new_pageblock->unallocated)) / size;
	

	new_pageblock->remotely_freed_list.next = NULL;
	new_pageblock->freed_list.next = NULL;

	if(mem.obj[position].active_tail != NULL){
		new_pageblock->prev = mem.obj[position].active_tail->prev;
		new_pageblock->next = mem.obj[position].active_tail;
		mem.obj[position].active_tail->prev->next = new_pageblock;
		mem.obj[position].active_tail->prev = new_pageblock;

	}
	else{
		new_pageblock->prev = new_pageblock;
		new_pageblock->next = new_pageblock;

		mem.obj[position].active_tail = new_pageblock;
		mem.obj[position].active_head = new_pageblock;
	}

	new_pageblock->heap = (local_heap_t *) &(mem.obj[position]);
}

void *my_malloc(size_t size){									// function used by users
	size_t object_size;
	void *address;
	int position;

	object_size = (int) pow(2, ceil(log(size)/log(2))); // todo change
	// printf("size of alloc: %zu, next pow of 2: %zu\n", size, object_size);

	position = object_class_exists(object_size);
	if( position == -1 || mem.obj[position].active_tail->num_free_objects == 0 ){
		printf("allocate_memory call\n");
		allocate_memory(object_size);
		position = get_object_class(object_size);
	}
	address = (void *) unstack( (node_t *) &mem.obj[position].active_tail->freed_list );
	if( address != NULL ){					// get from superblock
		printf("GOT FROM FREE LIST\n");
		return address;
	}

	else{							// get from free list
		printf("GOT FROM SUPERBLOCK\n");
		address = mem.obj[position].active_tail->unallocated;
		mem.obj[position].active_tail->unallocated = (void *) (((unsigned long) mem.obj[position].active_tail->unallocated) + object_size);
	}

				// MUST CHECK IF THERE IS NO ITEM IN FREE LIST AND NO OTHER MEMORY IN SUPERBLOCK

	return address;
}

int my_free(void *address){
	unsigned long mask;
	pageblock_t *my_pageblock;

	mask = ~(PAGEBLOCK_SIZE - 1);
	my_pageblock = (pageblock_t *) (mask & ((unsigned long)address));

	stack( (node_t *) &(my_pageblock->freed_list), (node_t *) address);
	
	printf("inserted to free %p\n", address);


	//printf("my_free:\n\tslot = %d, numOf8B = %d\n", i, numOf8B);
	return 0;
}
