#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

typedef struct node {
    struct node *next;
} node_t;

void stack(node_t *head, node_t *address);
void atomic_stack(volatile node_t *head, node_t *address);
node_t *unstack(node_t *head);
node_t *atomic_unstack(node_t *head);

#endif
