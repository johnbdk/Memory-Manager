#include "queue.h"

node_t *unstack(node_t *queue) {
    node_t *curr;

    if( queue->next == NULL ){
        return NULL;
    }

    curr = queue->next;
    queue->next = curr->next;

    return curr;
}

void stack(node_t *queue, node_t *element) {

    element->next = queue->next;
    queue->next = element;

}
