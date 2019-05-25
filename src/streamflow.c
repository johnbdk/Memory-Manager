#include "streamflow.h"


__thread local_heap_t mem = {{{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
					{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}}};

pageblock_t *cached_pageblock = NULL;

void push(pageblock_t *node) {
	node->next = cached_pageblock;
	node->prev = NULL;
	cached_pageblock = node;
}

pageblock_t* pop() {
	if (cached_pageblock == NULL)
		return NULL;

	pageblock_t *node = cached_pageblock;
	cached_pageblock = cached_pageblock->next;
	if (cached_pageblock != NULL)
		cached_pageblock->prev = NULL;

	return node;
}

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

	if(mem.obj[position].active_tail == NULL){
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

	new_pageblock = pop();
	if (new_pageblock != NULL) {
		printf("GET FROM CACHED LIST\n");
	}
	else {
		temp_addr = mmap(NULL, 2*allocate_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);			// allocate 1 page for each object size
		new_pageblock = (pageblock_t *) ( (((unsigned long)temp_addr) & page_block_mask)+ allocate_size );

		unsigned long temp_size =  ((unsigned long)new_pageblock) - ((unsigned long)temp_addr) ;
		if (temp_size != allocate_size) {
			munmap(temp_addr, temp_size );
			temp_size =  (((unsigned long)temp_addr) + 2*allocate_size) - ( ((unsigned long)new_pageblock) + allocate_size ) ;
			munmap( (void *) ( ((unsigned long)new_pageblock) + allocate_size ) , temp_size );
		}
		else {
			printf("SAVE CACHE PAGEBLOCK\n");
			fflush(stdout);
			push((pageblock_t *)temp_addr);
		}
	}
	

	new_pageblock->id = &(mem.obj[0]);
	new_pageblock->pageblock_size = allocate_size;
	new_pageblock->object_size = size;

	new_pageblock->unallocated = (void *) (( ((unsigned long) new_pageblock) + sizeof(pageblock_t) + mask ) & (~mask) );
	printf("~~~~~~ new_pageblock: %p\n", new_pageblock);

	new_pageblock->num_unalloc_objs =  (unsigned) ((((unsigned long) new_pageblock) + allocate_size) - ((unsigned long) new_pageblock->unallocated)) / size;
	new_pageblock->max_objs = new_pageblock->num_unalloc_objs;
	new_pageblock->num_freed_objs = 0;
	new_pageblock->num_alloc_objs = 0;
	new_pageblock->sloppy_counter = 0;

	new_pageblock->remotely_freed_list.next = NULL;
	new_pageblock->freed_list.next = NULL;

	if(mem.obj[position].active_head != NULL){
		mem.obj[position].active_tail = mem.obj[position].active_head;

		new_pageblock->prev = mem.obj[position].active_head;//->prev;
		new_pageblock->next = mem.obj[position].active_head->next;

		mem.obj[position].active_head->next->prev = new_pageblock;
		mem.obj[position].active_head->next = new_pageblock;

		mem.obj[position].active_head = new_pageblock;
	}
	else{
		new_pageblock->prev = new_pageblock;
		new_pageblock->next = new_pageblock;

		mem.obj[position].active_head = new_pageblock;
		mem.obj[position].active_tail = new_pageblock;
	}

	new_pageblock->heap = (object_class_t *) &(mem.obj[position]);
}

void *my_malloc(size_t size){									// function used by users
	size_t object_size;
	void *address;
	int position;

	object_size = (int) pow(2, ceil(log(size)/log(2))); // todo change
	// printf("size of alloc: %zu, next pow of 2: %zu\n", size, object_size);
	//printf("%zu\n", object_size);
	position = object_class_exists(object_size);
	if( position == -1 ){
		allocate_memory(object_size);
		position = get_object_class(object_size);

		printf("GOT FROM SUPERBLOCK\n");
		address = mem.obj[position].active_head->unallocated;
		mem.obj[position].active_head->unallocated = (void *) (((unsigned long) mem.obj[position].active_head->unallocated) + object_size);
		mem.obj[position].active_head->num_unalloc_objs--;
		mem.obj[position].active_head->num_alloc_objs++;
	}
	else if( mem.obj[position].active_head->num_freed_objs != 0 ){
		address = (void *) unstack( (node_t *) &mem.obj[position].active_head->freed_list );
		mem.obj[position].active_head->num_freed_objs--;
	}
	else{
		node_t *addrs;
		addrs = (node_t *) atomic_unstack( (node_t *) &mem.obj[position].active_head->remotely_freed_list );
		if(addrs == NULL){
			if( mem.obj[position].active_head->num_unalloc_objs == 0){
				allocate_memory(object_size);
				position = get_object_class(object_size);
			}
			printf("GOT FROM SUPERBLOCK\n");
			address = mem.obj[position].active_head->unallocated;
			mem.obj[position].active_head->unallocated = (void *) (((unsigned long) mem.obj[position].active_head->unallocated) + object_size);
			mem.obj[position].active_head->num_unalloc_objs--;
			mem.obj[position].active_head->num_alloc_objs++;
		}
		else{
			address = (void *) addrs;
			for(node_t *curr = addrs->next; curr != NULL; curr = curr->next){
				printf("%p\n",curr );
				stack( (node_t *) &(mem.obj[position].active_head->freed_list), curr);
				mem.obj[position].active_head->num_freed_objs++;
			}
		}
	}
	if( position == -1 || mem.obj[position].active_head->num_unalloc_objs == 0 ){
		printf("allocate_memory call\n");
		allocate_memory(object_size);
		position = get_object_class(object_size);
	}
	
	if( address != NULL ) {					// get from superblock
		printf("GOT FROM FREE LIST\n");
		
	}

	return address;
}

int my_free(void *address){
	unsigned long mask;
	pageblock_t *my_pageblock;

	mask = ~(PAGEBLOCK_SIZE - 1);
	my_pageblock = (pageblock_t *) (mask & ((unsigned long)address));

	if (my_pageblock->id == &(mem.obj[0])) {
		stack( (node_t *) &(my_pageblock->freed_list), (node_t *) address);
		printf("inserted to free %p\n", address);
		(my_pageblock->num_freed_objs)++;

		/* if freed objects are less than the allocated objects, then take from remotely_freed_list based on a threshold & sloppy_counter */
		if (my_pageblock->num_freed_objs <  my_pageblock->num_alloc_objs) {
			float threshold = 0.8;
			if (my_pageblock->sloppy_counter > threshold*(my_pageblock->num_alloc_objs - my_pageblock->num_freed_objs)) {
				node_t *addrs;
				addrs = (node_t *) atomic_unstack( (node_t *) &(my_pageblock->heap->active_head->remotely_freed_list) );
				my_pageblock->sloppy_counter = 0;
				if (addrs != NULL) {
					printf("PASSED THE THRESHOLD AND ATOMIC READ THE REMOTELY FREED LIST\n");
					for(node_t *curr = addrs->next; curr != NULL; curr = curr->next){
						stack( (node_t *) &(my_pageblock->heap->active_head->freed_list), curr);
						my_pageblock->heap->active_head->num_freed_objs++;
					}
				}
			}
		}

		/* if freed objects are equal to the allocated objects, then check if page block must be chached */
		if (my_pageblock->num_freed_objs == my_pageblock->num_alloc_objs) {
			unsigned neighbor_unused_objs;
			unsigned max_objs;
			if (my_pageblock == my_pageblock->heap->active_head) {			// if pageblock is head
				neighbor_unused_objs = my_pageblock->next->num_unalloc_objs + my_pageblock->next->num_freed_objs;
				max_objs = my_pageblock->next->max_objs;
			}
			else{															// if not head
				neighbor_unused_objs = my_pageblock->heap->active_head->num_unalloc_objs + my_pageblock->heap->active_head->num_freed_objs;
				max_objs = my_pageblock->heap->active_head->max_objs;
			}

			if (neighbor_unused_objs >= max_objs/2) {

				my_pageblock->prev->next = my_pageblock->next;				// change my neighbours' pointers
				my_pageblock->next->prev = my_pageblock->prev;

				if (my_pageblock == my_pageblock->heap->active_head) {		// if head make something else new head
					my_pageblock->heap->active_head = my_pageblock->next;
				}
				if (my_pageblock == my_pageblock->heap->active_tail) {		// if tail make something else new tail
					my_pageblock->heap->active_tail = my_pageblock->prev;
				}

				push(my_pageblock);
				printf("SAVE CACHE PAGEBLOCK FROM A SUPERBLOCK\n");
			}
		}
		
	}
	else {
		atomic_stack( (node_t *) &(my_pageblock->remotely_freed_list), (node_t *) address);
		(my_pageblock->sloppy_counter)++;
		printf("inserted to remote free %p\n", address);
	}

	//printf("my_free:\n\tslot = %d, numOf8B = %d\n", i, numOf8B);
	return 0;
}
