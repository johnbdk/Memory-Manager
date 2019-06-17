#include "streamflow.h"

__thread local_heap_t mem = {{{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
					{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}}};

__thread pageblock_t *cached_pageblock = NULL;

const int slots[OBJECT_CLASS] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048}; 

char magic_number[72] = "Themanagementoflargeobjectsissignificantlysimplerthanthatofsmallobjects";

void push_cached_pb(pageblock_t *node) {

	node->next = cached_pageblock;
	node->prev = NULL;
	cached_pageblock = node;
}

pageblock_t* pop_cached_pb() {
	pageblock_t *curr_node;

	if (cached_pageblock == NULL) {
		return NULL;
	}

	curr_node = cached_pageblock;
	cached_pageblock = cached_pageblock->next;
	if (cached_pageblock != NULL) {
		cached_pageblock->prev = NULL;
	}
	return curr_node;
}

int get_slot(int start, int end, int value, int *pos) {
	int index;

	index = (end - start) / 2;
	index += start;

	if (value > 2048) {
		if (pos != NULL) {
			*pos = -1;
		}
		return -1;
	}

	if (end == 0) {
		if (pos != NULL) {
			*pos = 0;
		}
		return slots[0];
	}

	if ((slots[index] >= value) && (slots[index - 1] < value)) {
		if (pos != NULL) {
			*pos = index;
		}
		return slots[index];
	}
	else if (slots[index] < value) {
		return get_slot(index + 1, end, value, pos);
	}
	else {
		return get_slot(start, index - 1, value, pos);
	}
}

int get_object_class(size_t size) {					// returns the position in array that the objects of a specific size should be placed
	int position;									// size 8 -> position 0, size 16 -> position 1, ...

	get_slot(0, OBJECT_CLASS, size, &position);
	return position;
}

int object_class_exists(size_t size) {				// if objects of this size already exist (there is already a slot in the array)
	int position;

	position = get_object_class(size);
	if(mem.obj[position].active_tail == NULL) {
		return -1;
	}
	return position;
}

int allocate_memory(size_t size) {	
	void *temp_addr;
	pageblock_t *new_pageblock;
	int position, allocate_size;
	unsigned long page_block_mask, mask = 15;

	position = get_object_class(size);
	allocate_size = PAGEBLOCK_SIZE;
	page_block_mask = ~(allocate_size - 1);
	new_pageblock = pop_cached_pb();
	if (new_pageblock == NULL) {
		/* allocate 1 page for each object size */
		temp_addr = mmap(NULL, 2 * allocate_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
		if (temp_addr == NULL) {
			return -1;
		}

		new_pageblock = (pageblock_t *) ((((unsigned long) temp_addr) & page_block_mask) + allocate_size);
		unsigned long temp_size = ((unsigned long) new_pageblock) - ((unsigned long) temp_addr);
		if (temp_size != allocate_size) {
			munmap(temp_addr, temp_size);
			temp_size =  (((unsigned long) temp_addr) + (2 * allocate_size)) - (((unsigned long) new_pageblock) + allocate_size);
			munmap((void *) (((unsigned long) new_pageblock) + allocate_size) , temp_size);
		}
		else {
			//printf("SAVE CACHE PAGEBLOCK\n");
			//fflush(stdout);
			push_cached_pb((pageblock_t *)temp_addr);
		}
	}

	new_pageblock->id = &(mem.obj[0]);
	new_pageblock->sloppy_counter = 0;
	new_pageblock->num_freed_objs = 0;
	new_pageblock->num_alloc_objs = 0;
	new_pageblock->object_size = size;
	new_pageblock->freed_list.next = NULL;
	new_pageblock->pageblock_size = allocate_size;
	new_pageblock->remotely_freed_list.next = NULL;
	new_pageblock->max_objs = new_pageblock->num_unalloc_objs;
	new_pageblock->unallocated = (void *) ((((unsigned long) new_pageblock) + sizeof(pageblock_t) + mask) & (~mask));
	new_pageblock->num_unalloc_objs = (unsigned) ((((unsigned long) new_pageblock) + allocate_size) - ((unsigned long) new_pageblock->unallocated)) / size;
	//printf("~~~~~~ new_pageblock: %p\n", new_pageblock);
	//fflush(stdout);

	if (mem.obj[position].active_head != NULL) {
		mem.obj[position].active_tail = mem.obj[position].active_head;
		new_pageblock->prev = mem.obj[position].active_head;
		new_pageblock->next = mem.obj[position].active_head->next;
		mem.obj[position].active_head->next->prev = new_pageblock;
		mem.obj[position].active_head->next = new_pageblock;
		mem.obj[position].active_head = new_pageblock;
	}
	else {
		new_pageblock->prev = new_pageblock;
		new_pageblock->next = new_pageblock;
		mem.obj[position].active_head = new_pageblock;
		mem.obj[position].active_tail = new_pageblock;
	}
	new_pageblock->heap = (object_class_t *) &(mem.obj[position]);
	return 1;
}

void *my_malloc(size_t size) {
	size_t object_size;
	void *address;
	int position;
	int ret_val;

	object_size = get_slot(0, OBJECT_CLASS, size, NULL);
	/* Allocate memory for large objects */
	if (object_size == -1) {
		//printf("LARGE OBJECT\n");
		address = mmap(NULL, size + sizeof(magic_number) + sizeof(unsigned long), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
		if (address == NULL) {
			return NULL;
		}
		unsigned long mmap_size = (unsigned long) (((size / PAGE) + 1) * 4096);
		memcpy(address, (void *) &mmap_size, sizeof(unsigned long));
		memcpy((void *) (((unsigned long) address) + sizeof(unsigned long)), (void *) magic_number, sizeof(magic_number));
		void *ret_address = (void *) (((unsigned long) address) + sizeof(magic_number) + sizeof(unsigned long));
		return ret_address; 
	}

	position = object_class_exists(object_size);
	if (position == -1 ) {
		ret_val = allocate_memory(object_size);
		if (ret_val == -1) {
			return NULL;
		}
		position = get_object_class(object_size);

		//printf("GOT FROM SUPERBLOCK\n");
		//fflush(stdout);
		address = mem.obj[position].active_head->unallocated;
		mem.obj[position].active_head->unallocated = (void *) (((unsigned long) mem.obj[position].active_head->unallocated) + object_size);
		mem.obj[position].active_head->num_unalloc_objs--;
		mem.obj[position].active_head->num_alloc_objs++;
	}
	else if (mem.obj[position].active_head->num_freed_objs != 0) {
		address = (void *) unstack((node_t *) &mem.obj[position].active_head->freed_list);
		mem.obj[position].active_head->num_freed_objs--;
	}
	else {
		node_t *addrs;

		addrs = (node_t *) atomic_unstack((node_t *) &mem.obj[position].active_head->remotely_freed_list);
		if (addrs == NULL) {
			if (mem.obj[position].active_head->num_unalloc_objs == 0) {
				ret_val = allocate_memory(object_size);
				if (ret_val == -1) {
					return NULL;
				}
				position = get_object_class(object_size);
			}
			//printf("GOT FROM SUPERBLOCK\n");
			//fflush(stdout);
			address = mem.obj[position].active_head->unallocated;
			mem.obj[position].active_head->unallocated = (void *) (((unsigned long) mem.obj[position].active_head->unallocated) + object_size);
			mem.obj[position].active_head->num_unalloc_objs--;
			mem.obj[position].active_head->num_alloc_objs++;
		}
		else {
			address = (void *) addrs;
			for (node_t *curr = addrs->next; curr != NULL; curr = curr->next) {
				//printf("%p\n",curr );
				stack( (node_t *) &(mem.obj[position].active_head->freed_list), curr);
				mem.obj[position].active_head->num_freed_objs++;
			}
		}
	}

	if (position == -1 || mem.obj[position].active_head->num_unalloc_objs == 0) {
		//printf("allocate_memory call\n");
		//fflush(stdout);
		ret_val = allocate_memory(object_size);
		if (ret_val == -1) {
			return NULL;
		}
		position = get_object_class(object_size);
	}
	
	/* Get from superblock */
	if (address != NULL) {
		//printf("GOT FROM FREE LIST\n");
	}
	return address;
}

void my_free(void *address){
	unsigned long mask;
	pageblock_t *my_pageblock;
	char is_it_magic[72];

	memcpy((void *) is_it_magic, (void *) (((unsigned long) address) - sizeof(magic_number)), sizeof(magic_number));
	
	if (memcmp((void *) magic_number, (void *) is_it_magic, sizeof(magic_number)) == 0) {
		unsigned long mmap_size;
		//printf("LARGE OBJECT FREE\n");
		//fflush(stdout);
		mmap_size = *((unsigned long *) (((unsigned long) address) - sizeof(magic_number) - sizeof(unsigned long)));
		munmap((void *) (((unsigned long) address) - sizeof(magic_number) - sizeof(unsigned long)), mmap_size);
		return;
	}

	mask = ~(PAGEBLOCK_SIZE - 1);
	my_pageblock = (pageblock_t *) (mask & ((unsigned long) address));

	if (my_pageblock->id == &(mem.obj[0])) {
		stack( (node_t *) &(my_pageblock->freed_list), (node_t *) address);
		//printf("inserted to free %p\n", address);
		//fflush(stdout);
		(my_pageblock->num_freed_objs)++;

		/* if freed objects are less than the allocated objects, then take from remotely_freed_list based on a threshold & sloppy_counter */
		if (my_pageblock->num_freed_objs < my_pageblock->num_alloc_objs) {
			float threshold = 0.8;

			if (my_pageblock->sloppy_counter > threshold*(my_pageblock->num_alloc_objs - my_pageblock->num_freed_objs)) {
				node_t *addrs = (node_t *) atomic_unstack((node_t *) &(my_pageblock->remotely_freed_list));
				my_pageblock->sloppy_counter = 0;

				if (addrs != NULL) {
					node_t * curr, *temp;
					for(curr = addrs, temp = addrs->next; curr != NULL;) {
						stack( (node_t *) &(my_pageblock->heap->active_head->freed_list), curr);
						my_pageblock->heap->active_head->num_freed_objs++;
						curr = temp;
						if (temp != NULL) {
							temp = temp->next;
						}
					}
				}
			}
		}

		/* if freed objects are equal to the allocated objects, then check if page block must be chached */
		if (my_pageblock->num_freed_objs == my_pageblock->num_alloc_objs) {
			unsigned neighbor_unused_objs;
			unsigned max_objs;

			if (my_pageblock == my_pageblock->heap->active_head) {	// if pageblock is head
				neighbor_unused_objs = my_pageblock->next->num_unalloc_objs + my_pageblock->next->num_freed_objs;
				max_objs = my_pageblock->next->max_objs;
			}
			else {	// if not head
				neighbor_unused_objs = my_pageblock->heap->active_head->num_unalloc_objs + my_pageblock->heap->active_head->num_freed_objs;
				max_objs = my_pageblock->heap->active_head->max_objs;
			}

			if (neighbor_unused_objs >= (max_objs / 2)) {
				my_pageblock->prev->next = my_pageblock->next;				// change my neighbours' pointers
				my_pageblock->next->prev = my_pageblock->prev;

				if (my_pageblock == my_pageblock->heap->active_head) {		// if head make something else new head
					my_pageblock->heap->active_head = my_pageblock->next;
				}
				if (my_pageblock == my_pageblock->heap->active_tail) {		// if tail make something else new tail
					my_pageblock->heap->active_tail = my_pageblock->prev;
				}
				push_cached_pb(my_pageblock);
				//printf("SAVE CACHE PAGEBLOCK FROM A SUPERBLOCK\n");
				//fflush(stdout);
			}
		}
	}
	else {
		atomic_stack((node_t *) &(my_pageblock->remotely_freed_list), (node_t *) address);
		(my_pageblock->sloppy_counter)++;
		//printf("inserted to remote free %p\n", address);
		//fflush(stdout);
	}
}
