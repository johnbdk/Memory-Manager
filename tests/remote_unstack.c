#include "../src/streamflow.h"
#include <pthread.h>

int *ad[10000];
volatile int counter;
int *ad2[10000];
volatile int counter2;


void *job() {
	int random;

	// srand(time(NULL));

	for(int i=0; i<10000; i++){
		random = 100;//(rand() + 1)%2048;
		ad[i] = (int *) malloc(random*sizeof(char));
		counter++;
		if(i == 1000 || i == 3000){
			sleep(0.5);
		}
	}
	for(int i=5000; i<10000; i++){
		free((void *) ad[i]);
	}

	return NULL;
}

void *job2() {
	int i=0;

	while(i < 5000){
		while(i >= counter);

		printf("will free small\n");
		fflush(stdout);
		free((void *) ad[i]);
		i++;
		
		if( i == 2500){
			sleep(0.5);
		}
	}

	return NULL;
}

void *job3() {

	for(int i=0; i<10000; i++){
		ad2[i] = (int *) malloc(3000);
 		counter2 += 1;
 	// 	if(i == 500 || i == 2500){
		// 	sleep(0.5);
		// }
	}

	return NULL;
}

void *job4() {
	int i=0;

	// int j = 0;
	while(i < 10000){
		// printf("HERE %d\n",j++);
		while(i >= counter2);
		printf("will free large\n");
		fflush(stdout);
		free((void *) ad2[i]);
		i++;
		
	}

	return NULL;
}


int main(int argc, char* argv[]) {
	pthread_t x,y,z,w;

	counter = 0;
	counter2 = 0;

	pthread_create(&y, NULL, job2, NULL);
	pthread_create(&x, NULL, job, NULL);
	pthread_create(&w, NULL, job4, NULL);
	pthread_create(&z, NULL, job3, NULL);

	pthread_join(x, NULL);
	pthread_join(y, NULL);
	pthread_join(z, NULL);
	pthread_join(w, NULL);

	return 0;
}