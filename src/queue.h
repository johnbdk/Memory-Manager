#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <unistd.h>

typedef struct node {
    struct node *next;
} node_t;

node_t *unstack(node_t *head);
void stack(node_t *head, node_t *address);

#endif
