#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
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
    // lock_t lock;
} queue_t;

// int queue_empty(queue_t *head);
// void print_queue(queue_t *head);
// void enqueue_head(queue_t *head, node_t *element);
// void enqueue_tail(queue_t *head, node_t *element);
queue_t *queue_create();
// node_t *dequeue_head(queue_t *head);
// node_t *dequeue_tail(queue_t *head);
void *dequeue_head_no_lock(queue_t *head);
void enqueue_head_no_lock(queue_t *head, void *address);

#endif