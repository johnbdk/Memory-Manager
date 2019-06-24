#include "streamflow.h"

__thread pageblock_t *cached_pageblock = NULL;
__thread local_heap_t mem = {{{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
					{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}}};
					
const int slots[OBJECT_CLASS] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048}; 
int no_cached_pb = 0;
char magic_number[72] = "Themanagementoflargeobjectsissignificantlysimplerthanthatofsmallobjects";

#ifdef METRICS
long object_metric = 0;
long mmap_metric = 0;
unsigned long long seq = 0;
#endif

void push_cached_pb(pageblock_t *node) {

	node->next = cached_pageblock;
	node->prev = NULL;
	cached_pageblock = node;
	no_cached_pb++;
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
	no_cached_pb--;
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
		if (temp_addr == MAP_FAILED) {
			return -1;
		}
#ifdef METRICS
		__sync_fetch_and_add(&mmap_metric, 2*allocate_size);
#endif

		new_pageblock = (pageblock_t *) ((((unsigned long) temp_addr) & page_block_mask) + allocate_size);
		unsigned long temp_size_1, temp_size_2;
		temp_size_1 = ((unsigned long) new_pageblock) - ((unsigned long) temp_addr);
		if (temp_size_1 != allocate_size) {
			munmap(temp_addr, temp_size_1);
			temp_size_2 =  (((unsigned long) temp_addr) + (2 * allocate_size)) - (((unsigned long) new_pageblock) + allocate_size);
			munmap((void *) (((unsigned long) new_pageblock) + allocate_size) , temp_size_2);
#ifdef METRICS
			__sync_fetch_and_sub(&mmap_metric, temp_size_1 + temp_size_2);
#endif
		}
		else {
			/* If we have free slots in cache */
			if (no_cached_pb < MAX_CACHED_PB) {
#ifdef DBUG
				printf("SAVE CACHE PAGEBLOCK\n");
				fflush(stdout);
#endif
				push_cached_pb((pageblock_t *) temp_addr);
			}
			/* Cache is full, return address to OS */
			else {
				munmap((void *) (pageblock_t *) temp_addr, PAGEBLOCK_SIZE);
#ifdef METRICS
				__sync_fetch_and_sub(&mmap_metric, PAGEBLOCK_SIZE);
#endif
			}
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
#ifdef DBUG
	printf("~~~~~~ new_pageblock: %p\n", new_pageblock);
	fflush(stdout);
#endif

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
#ifdef DBUG
		printf("LARGE OBJECT\n");
		fflush(stdout);
#endif
		/* Doesn't need to add metrics to large objects, is 1-to-1 */
		address = mmap(NULL, size + sizeof(magic_number) + sizeof(unsigned long), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
		if (address == MAP_FAILED) {
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
#ifdef DBUG
		printf("GOT FROM SUPERBLOCK\n");
		fflush(stdout);
#endif
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
#ifdef DBUG
			printf("GOT FROM SUPERBLOCK\n");
			fflush(stdout);
#endif
			address = mem.obj[position].active_head->unallocated;
			mem.obj[position].active_head->unallocated = (void *) (((unsigned long) mem.obj[position].active_head->unallocated) + object_size);
			mem.obj[position].active_head->num_unalloc_objs--;
			mem.obj[position].active_head->num_alloc_objs++;
		}
		else {
			node_t * curr, *temp;
			/* The address to return, to the rest of them just add them in free list */
			address = (void *) addrs;
			/* Take the next address (if not NULL) and start save the rest of them in free list */
			if (addrs->next == NULL) {
				return address;
			}
			addrs = addrs->next;
			for (curr = addrs, temp = addrs->next; curr != NULL;) {
				stack((node_t *) &(mem.obj[position].active_head->freed_list), curr);
				mem.obj[position].active_head->num_freed_objs++;
				curr = temp;
				if (temp != NULL) {
					temp = temp->next;
				}
			}
		}
	}

	if (position == -1 || mem.obj[position].active_head->num_unalloc_objs == 0) {
#ifdef DBUG
		printf("Allocate_memory call\n");
		fflush(stdout);
#endif
		ret_val = allocate_memory(object_size);
		if (ret_val == -1) {
			return NULL;
		}
		position = get_object_class(object_size);
	}

#ifdef DBUG	
	/* Get from superblock */
	if (address != NULL) {
		printf("GOT FROM FREE LIST\n");
		fflush(stdout);
	}
#endif
	/* Metrics for small objects */
#ifdef METRICS
	__sync_fetch_and_add(&object_metric, object_size);
	seq++;
	// fprintf(stderr, "***************************************************\n");
	// fprintf(stderr, "SEQ %llu, MMAP Bytes:\t%.2lf\tObject Bytes: %ld\n", seq, mmap_metric, object_metric);
	// fprintf(stderr, "SEQ %llu, RATIO (MMAP/Objects):\t%lf\n\n", seq, (double) (mmap_metric / object_metric));
	fprintf(stderr, "%ld, %ld, %lf\n", mmap_metric, object_metric,  ((double)mmap_metric / (double)object_metric));
	fflush(stderr);

	// fprintf(stderr, "***************************************************\n\n");
	fflush(stderr);
#endif
	return address;
}

void my_free(void *address) {
	unsigned long mask;
	pageblock_t *my_pageblock;
	char is_it_magic[72];

	memcpy((void *) is_it_magic, (void *) (((unsigned long) address) - sizeof(magic_number)), sizeof(magic_number));
	
	if (memcmp((void *) magic_number, (void *) is_it_magic, sizeof(magic_number)) == 0) {
		unsigned long mmap_size;
#ifdef DBUG
		printf("LARGE OBJECT FREE\n");
		fflush(stdout);
#endif
		mmap_size = *((unsigned long *) (((unsigned long) address) - sizeof(magic_number) - sizeof(unsigned long)));
		munmap((void *) (((unsigned long) address) - sizeof(magic_number) - sizeof(unsigned long)), mmap_size);
		return;
	}

	mask = ~(PAGEBLOCK_SIZE - 1);
	my_pageblock = (pageblock_t *) (mask & ((unsigned long) address));

	if (my_pageblock->id == &(mem.obj[0])) {
		stack( (node_t *) &(my_pageblock->freed_list), (node_t *) address);
#ifdef DBUG
		printf("inserted to free %p\n", address);
		fflush(stdout);
#endif
		(my_pageblock->num_freed_objs)++;

		/* if freed objects are less than the allocated objects, then take from remotely_freed_list based on a threshold & sloppy_counter */
		if (my_pageblock->num_freed_objs < my_pageblock->num_alloc_objs) {
			float threshold = 0.8;

			if (my_pageblock->sloppy_counter > threshold*(my_pageblock->num_alloc_objs - my_pageblock->num_freed_objs)) {
				node_t *addrs = (node_t *) atomic_unstack((node_t *) &(my_pageblock->remotely_freed_list));
				my_pageblock->sloppy_counter = 0;

				if (addrs != NULL) {
					node_t * curr, *temp;
					for (curr = addrs, temp = addrs->next; curr != NULL;) {
						stack((node_t *) &(my_pageblock->heap->active_head->freed_list), curr);
						my_pageblock->heap->active_head->num_freed_objs++;
						curr = temp;
						if (temp != NULL) {
							temp = temp->next;
						}
					}
				}
			}
		}

		/* if freed objects are equal to the allocated objects, then check if page block must be cached */
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

				/* If we have free slots in cache */
				if (no_cached_pb < MAX_CACHED_PB) {
					push_cached_pb(my_pageblock);
#ifdef DBUG
					printf("SAVE CACHE PAGEBLOCK FROM A SUPERBLOCK\n");
					fflush(stdout);
#endif
				}
				/* Cache is full, return address to OS */
				else {
					munmap((void *) my_pageblock, PAGEBLOCK_SIZE);
#ifdef METRICS
					__sync_fetch_and_sub(&mmap_metric, PAGEBLOCK_SIZE);		
#endif
#ifdef DBUG
					printf("UNMMAP PAGEBLOCK FROM A SUPERBLOCK, CACHE IS FULL\n");
					fflush(stdout);
#endif
				}
			}
		}
	}
	else {
		atomic_stack((node_t *) &(my_pageblock->remotely_freed_list), (node_t *) address);
		(my_pageblock->sloppy_counter)++;
#ifdef DBUG
		printf("Inserted to remote free %p\n", address);
		fflush(stdout);
#endif
	}

#ifdef METRICS
	__sync_fetch_and_add(&object_metric, -my_pageblock->object_size);
#endif
}
