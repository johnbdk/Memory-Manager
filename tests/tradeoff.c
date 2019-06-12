#include "../src/streamflow.h"
#include <pthread.h>
#include "../src/lock.h"

#define SLOTS 9
#define ELEMS 500

typedef struct {
    char *buf[ELEMS];
    int occupied;
    lock_t lock;
    lock_t free_lock;
} buffer_t;

buffer_t b;
int slot_sz[SLOTS] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};

void *producer()
{
	printf("PRODUCER: start\n");
	fflush(stdout);
	lock_acquire(&b.lock);

	for (int j = 0; j < ELEMS; j++) {
		b.buf[j] = (char*)malloc(slot_sz[8]*sizeof(char));
		b.occupied++;
	}
	printf("PRODUCER: allocated elements\n");
	fflush(stdout);
    	
	lock_release(&b.lock);
    lock_acquire(&b.free_lock);

    for (int i = 0; i < 40; i++)
        free(b.buf[i]);
    return NULL;
}

void *consumer()
{
    lock_acquire(&b.free_lock);

	if(b.occupied != ELEMS) {
		printf("CONSUMER: locked\n");
		fflush(stdout);
		while(!b.occupied);
    	lock_acquire(&b.lock);
	}
	printf("CONSUMER: un-locked\n");
	fflush(stdout);
    	
    // remote free
    for (int j = 40; j < ELEMS; j++) {
    	free(b.buf[j]);
    	b.occupied--;
    }
    printf("CONSUMER: freed \n");
    lock_release(&b.free_lock);
	fflush(stdout);
    return NULL;
}

int main(int argc, char* argv[]){
	pthread_t x, y;

	lock_init(&b.lock);
    lock_init(&b.free_lock);
	b.occupied = 0;

	pthread_create(&x, NULL, producer, NULL);
	pthread_create(&y, NULL, consumer, NULL);

	pthread_join(x, NULL);
	pthread_join(y, NULL);
	return 0;
}
