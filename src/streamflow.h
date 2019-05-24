#ifndef STREAMFLOW_H
#define STREAMFLOW_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "queue.h"

#define OBJECT_CLASS 9
#define PAGE sysconf(_SC_PAGE_SIZE)
#define PAGEBLOCK_SIZE PAGE*8
#define CACHE_LINE sysconf(_SC_LEVEL1_DCACHE_LINESIZE)
#define max(x, y) (((x) > (y)) ? (x) : (y))

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct object_class {
	struct pageblock *active_head;
	struct pageblock *active_tail;
};

struct local_heap {
	struct object_class obj[OBJECT_CLASS];
};

struct pageblock {
	struct object_class *id;
	unsigned pageblock_size;
	unsigned object_size;
	unsigned num_unalloc_objs;		// num of objects that are not used
	unsigned num_alloc_objs;		// num of objects that used 
	unsigned num_freed_objs;		// num of objects that freed (not remotely)
	unsigned max_objs;
	node_t freed_list;				// start of free list
	void *unallocated;				// points to the next unallocated space within a pageblock
	node_t remotely_freed_list;
	struct pageblock *next;
	struct pageblock *prev;
	struct object_class *heap;
};


typedef struct object_class object_class_t;
typedef struct local_heap local_heap_t;
typedef struct pageblock pageblock_t;

int my_free(void *address);
int get_object_class(size_t obj_size);
int object_class_exists(size_t obj_size);
void *my_malloc(size_t size);
void allocate_memory(size_t obj_size);

#endif
