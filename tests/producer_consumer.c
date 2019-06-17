#include "../src/streamflow.h"
#include <pthread.h>
#include "../src/lock.h"

#define SLOTS 9
#define ELEMS 2500

typedef struct {
    char *buf[ELEMS];
    int occupied;
    lock_t lock;
} buffer_t;

buffer_t b[SLOTS];
int slot_sz[SLOTS] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};

void *producer() {

    for (int i = 0; i < SLOTS; i++) {

    	printf("PRODUCER: slot %d\n", i);
		fflush(stdout);
    	lock_acquire(&b[i].lock);
    	printf("%d\n", i);
    	fflush(stdout);

    	for (int j = 0; j < ELEMS; j++) {
    		b[i].buf[j] = (char *) malloc(slot_sz[i] * sizeof(char));
    		b[i].occupied++;
    	}
    	printf("PRODUCER: allocated slot %d\n", i);
		fflush(stdout);
    	lock_release(&b[i].lock);
    }
    return NULL;
}

void *consumer() {

    for (int i = 0; i < SLOTS; i++) {
    	if (b[i].occupied != ELEMS) {
    		printf("CONSUMER: locked slot %d\n", i);
    		fflush(stdout);
    		while (!b[i].occupied);
        	lock_acquire(&b[i].lock);
	    	printf("%d\n", i);
	    	fflush(stdout);
    	}
    	printf("CONSUMER: un-locked slot %d\n", i);
		fflush(stdout);
        	
        // remote free
        for (int j = 0; j < ELEMS; j++) {
        	free(b[i].buf[j]);
        	b[i].occupied--;
        }
        printf("CONSUMER: freed slot %d\n", i);
		fflush(stdout);		
	}
    return NULL;
}

int main(int argc, char* argv[]) {
	pthread_t x, y;

	for (int i = 0; i < SLOTS; i++) {
		lock_init(&b[i].lock);
		b[i].occupied = 0;
	}

	pthread_create(&x, NULL, producer, NULL);
	pthread_create(&y, NULL, consumer, NULL);
    
	pthread_join(x, NULL);
	pthread_join(y, NULL);
	return 0;
}
