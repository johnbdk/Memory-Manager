#ifndef MEM_MANAGEMET_H
#define MEM_MANAGEMET_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "queue.h"

#define SLOTS 10
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define max(x, y) (((x) > (y)) ? (x) : (y))

typedef struct sblock {
	void *address;					// start of superblock
	int next_address;				// next address to return to user
	void *free_list;				// start of free list
} superblock;

superblock my_mem[SLOTS];				// 10 slots for objects 8, 16, 32, ... 4096

void mem_init();
int pos_of_object(size_t obj_size);
int object_size_exists(size_t obj_size);
void mem_allocate(size_t obj_size);
void *my_malloc(size_t size);
int my_free(void *address);

#endif
