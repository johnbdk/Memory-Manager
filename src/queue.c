#include "queue.h"

node_t *unstack(node_t *queue) {
    node_t *curr;

    if (queue->next == NULL ) {
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

void atomic_stack(volatile node_t *queue, node_t *element) {
	node_t *temp, *ret_val;

	while(1) {
		temp = queue->next;
		ret_val = __sync_val_compare_and_swap(&queue->next, temp, element);
		if (ret_val == temp) {
			element->next = ret_val;
			break;
		}
	}
}

node_t *atomic_unstack(node_t *queue) {
    node_t *curr;
    node_t *temp, *ret_val;

    if (queue->next == NULL) {
        return NULL;
    }

    while(1) {
		temp = queue->next;
		ret_val = __sync_val_compare_and_swap(&queue->next, temp, NULL);
		if (ret_val == temp) {
			curr = ret_val;
			return curr;
		}
	}
	return NULL;
}
