#include "queue.h"

queue_t *queue_create() {
    queue_t *queue;

    queue = (queue_t *) mmap(NULL, sizeof(queue_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    queue->head = (node_t *) mmap(NULL, sizeof(node_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    queue->head->next = queue->head;
    queue->head->prev = queue->head;
    queue->head->address = NULL;
    return queue;
}

void *dequeue_head(queue_t *queue) {
    node_t *curr;

    if ((queue->head->next == queue->head) || (queue->head == NULL)) {
        return NULL;
    }

    curr = queue->head->next;
    queue->head->next = curr->next;
    curr->next->prev = queue->head;
    return curr->address;
}

void enqueue_head(queue_t *queue, void *address) {
    node_t *element = (node_t *) mmap(NULL, sizeof(node_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);

    if (queue->head == NULL) {
        return;
    }

    element->address = address;
    element->next = queue->head->next;
    element->prev = queue->head;
    queue->head->next->prev = element;
    queue->head->next = element;
}
