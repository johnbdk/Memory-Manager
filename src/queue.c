#include "queue.h"

queue_t *queue_create() {
    queue_t *queue;

    queue = (queue_t *) mmap(NULL, sizeof(queue_t), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);
    queue->head = (node_t *) mmap(NULL, sizeof(node_t), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);
    queue->head->next = queue->head;
    queue->head->prev = queue->head;
    queue->head->address = NULL;
    // lock_init(&(queue->lock));

    return queue;
}

// int queue_empty(queue_t *queue) {
//     return (queue->head->next == queue->head) || (queue->head == NULL);
// }

// void enqueue_head(queue_t *queue, node_t *element) {

//     lock_acquire(&(queue->lock));
//     if (queue->head == NULL) {
//         lock_release(&(queue->lock));
//         return;
//     }
    
//     element->next = queue->head->next;
//     element->prev = queue->head;
//     queue->head->next->prev = element;
//     queue->head->next = element;
//     lock_release(&(queue->lock));
// }

// void enqueue_tail(queue_t *queue, node_t *element) {

//     lock_acquire(&(queue->lock));
//     if (queue->head == NULL) {
//         lock_release(&(queue->lock));
//         return;
//     }
    
//     element->next = queue->head;
//     element->prev = queue->head->prev;
//     queue->head->prev->next = element;
//     queue->head->prev = element;
//     lock_release(&(queue->lock));
// }

// node_t *dequeue_tail(queue_t *queue) {
//     node_t *curr;

//     lock_acquire(&(queue->lock));
//     if ((queue->head->next == queue->head) || (queue->head == NULL)) {
//         lock_release(&(queue->lock));
//         return NULL;
//     }

//     curr = queue->head->prev;
//     curr->prev->next = queue->head;
//     queue->head->prev = curr->prev;
//     lock_release(&(queue->lock));

//     return curr;
// }

// node_t *dequeue_head(queue_t *queue) {
//     node_t *curr;

//     lock_acquire(&(queue->lock));
//     if ((queue->head->next == queue->head) || (queue->head == NULL)) {
//         lock_release(&(queue->lock));
//         return NULL;
//     }

//     curr = queue->head->next;
//     queue->head->next = curr->next;
//     curr->next->prev = queue->head;
//     lock_release(&(queue->lock));

//     return curr;
// }

void *dequeue_head_no_lock(queue_t *queue) {
    node_t *curr;

    if ((queue->head->next == queue->head) || (queue->head == NULL)) {
        return NULL;
    }

    curr = queue->head->next;
    queue->head->next = curr->next;
    curr->next->prev = queue->head;

    return curr->address;
}

void enqueue_head_no_lock(queue_t *queue, void *address) {
    node_t *element = (node_t *) mmap(NULL, sizeof(node_t), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);

    if (queue->head == NULL) {
        return;
    }

    element->address = address;
    element->next = queue->head->next;
    element->prev = queue->head;
    queue->head->next->prev = element;
    queue->head->next = element;
}
