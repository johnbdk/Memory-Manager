#include "../src/streamflow.h"
#include <pthread.h>

int *ad[10000];
volatile int counter;
int *ad2[10000];
int *ad3[10000];
volatile int counter2;
volatile int counter3;


void *job() {

	for(int i=0; i<10000; i++){
		ad[i] = (int *) malloc(100*sizeof(char));
		counter++;
		if(i == 1000 || i == 3000){
			sleep(0.5);
		}
	}
	for(int i=1; i<10000; i += 2){
		free((void *) ad[i]);
		ad[i] = (int *) malloc(100*sizeof(char));
	}

	return NULL;
}

void *job2() {
	int i=0;

	while(i < 10000){
		while(i >= counter);

		// printf("will free small\n");
		fflush(stdout);
		free((void *) ad[i]);
		ad[i] = (int *) malloc(100*sizeof(char));
		i += 2;
		
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
		// printf("will free large\n");
		fflush(stdout);
		free((void *) ad2[i]);
		i++;
		
	}

	return NULL;
}

void *job5() {
	int random;

	// srand(time(NULL));


	for(int i=0; i<10000; i++){
		random = (rand() + 1)%2048;
		ad3[i] = (int *) malloc(random*sizeof(char));
		counter3++;
	}
	for(int i=1; i<10000; i += 2){
		free((void *) ad3[i]);
		random = (rand() + 1)%2048;
		ad3[i] = (int *) malloc(random*sizeof(char));
	}

	return NULL;
}

void *job6() {
	int i=0;
	int random;

	// srand(time(NULL));

	while(i < 10000){
		while(i >= counter3);

		// printf("will free small\n");
		fflush(stdout);
		free((void *) ad3[i]);
		random = (rand() + 1)%2048;
		ad3[i] = (int *) malloc(random*sizeof(char));
		i += 2;
	}

	return NULL;
}

int main(int argc, char* argv[]) {
	pthread_t x,y,z,w,q,o;

	counter = 0;
	counter2 = 0;
	counter3 = 0;

	pthread_create(&y, NULL, job2, NULL);
	pthread_create(&x, NULL, job, NULL);
	pthread_create(&w, NULL, job4, NULL);
	pthread_create(&z, NULL, job3, NULL);
	pthread_create(&q, NULL, job5, NULL);
	pthread_create(&o, NULL, job6, NULL);

	pthread_join(x, NULL);
	pthread_join(y, NULL);
	pthread_join(z, NULL);
	pthread_join(w, NULL);
	pthread_join(q, NULL);
	pthread_join(o, NULL);

	return 0;
}