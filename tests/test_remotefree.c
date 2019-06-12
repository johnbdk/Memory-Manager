#include "../src/streamflow.h"
#include <pthread.h>

int *ad2[2000];

void *job1() {
	char *ad1;

	srand(time(NULL));

	for(int i=0; i<2000; i++) {
		printf("T-1: ");
		ad1 = (char *) malloc(120*sizeof(char));
		ad1[0] = 1;
	}

	printf("remote access\n");
	fflush(stdout);
	printf("T-1: %d --> ", ad2[0][0]);
	// ad2[0][0] = 1;
	// printf("%d\n", ad2[0][0]);
	// free(ad2[0]);
	return NULL;
}

void *job2() {
	
	for(int i=0; i<6000; i++){
		if (i < 2000) {
			printf("T-2: ");
			ad2[i] = (int *) malloc(2*sizeof(int));
			ad2[i][0] = 2;
		}
		else if (i < 4000) {
			if (i-2000 != 0) {
				printf("T-2: free [%d] ", i-2000);
				free((void *) ad2[i-2000]);
			}
			else {
				printf("T-2: start freeing\n");
				fflush(stdout);
			}
		}
		/*else{
			ad2[i-4000] = (int *) malloc(100*sizeof(int));
		}*/
	}
	return NULL;
}

int main(int argc, char* argv[]){
	pthread_t x, y;
	pthread_create(&x, NULL, job1, NULL);
	pthread_create(&y, NULL, job2, NULL);

	pthread_join(x, NULL);
	pthread_join(y, NULL);
	return 0;
}
