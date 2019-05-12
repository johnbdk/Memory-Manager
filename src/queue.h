#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

typedef struct node {
    struct node *next;
    struct node *prev;
    void *address;
} node_t;

typedef struct queue {
	node_t *head;
} queue_t;

queue_t *queue_create();
void *dequeue_head(queue_t *head);
void enqueue_head(queue_t *head, void *address);
int search_queue(queue_t *queue, void *address);

#endif
