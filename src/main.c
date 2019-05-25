#include "streamflow.h"
#include <pthread.h>

void *job(){
	char *ad;
	int random;

	srand(time(NULL));

	for(int i=0; i<2000; i++){
		random = rand()%2048;
		ad = (char *) my_malloc(random*sizeof(char));
	}

	return NULL;
}

void *job2(){
	int *ad[2000];

	for(int i=0; i<6000; i++){
		if (i < 2000) {
			ad[i] = (int *) my_malloc(100*sizeof(int));
		}
		else if (i < 4000) {
			my_free((void *) &ad[i-2000]);
		}
		else{
			ad[i-4000] = (int *) my_malloc(100*sizeof(int));
		}
	}

	return NULL;
}

void *job3(){
	int *ad[2000];

	for(int i=0; i<6000; i++){
		if (i < 2000) {
			ad[i] = (int *) my_malloc(300*sizeof(int));
		}
		else if (i < 4000) {
			my_free((void *) &ad[i-2000]);
		}
		else{
			ad[i-4000] = (int *) my_malloc(300*sizeof(int));
		}
	}

	return NULL;
}

void *job4(){
	int *ad[2000];

	for(int i=0; i<6000; i++){
		if (i < 2000) {
			ad[i] = (int *) my_malloc(20*sizeof(int));
		}
		else if (i < 4000) {
			my_free((void *) &ad[i-2000]);
		}
		else{
			ad[i-4000] = (int *) my_malloc(20*sizeof(int));
		}
	}

	return NULL;
}

int main(int argc, char* argv[]){
	pthread_t x,y,z,w;
	pthread_create(&x, NULL, job, NULL);
	pthread_create(&y, NULL, job2, NULL);
	pthread_create(&z, NULL, job3, NULL);
	pthread_create(&w, NULL, job4, NULL);

	pthread_join(x, NULL);
	pthread_join(y, NULL);
	pthread_join(z, NULL);
	pthread_join(w, NULL);

	return 0;
}