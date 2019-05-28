#include "../src/streamflow.h"

int main(int argc, char* argv[]){
	int *ad[4*1024];
	int *ad2;

	// void *temp_addr = (void *) (mask & ((unsigned long)&address));
	for (int i = 0; i < 4*1024; i++) {
		ad[i] = (int *) my_malloc(2*sizeof(int)); //8 bytes -> slot 0
		ad[i][0] = 99;
		ad[i][1] = 782;
		fflush(stdout);
	}
	printf("go to second allocation loop\n");
	fflush(stdout);
	for (int i = 0; i < 1024; i++) {
		ad2 = (int *) my_malloc(sizeof(int));  //4 bytes -> slot 0
		ad2[0] = 45;
	}

	printf("%d --- %p\n", ad2[0], ad2 );
	printf("%d, %d --- %p\n", ad[0][0], ad[0][1], ad[0]);

	printf("my_free()\n");
	fflush(stdout);
	for(int i = 0; i < 4*1024; i++)
		my_free((void *) ad[i]);

	printf("reallocate\n");
	for (int i = 0; i < 4*1024; i++) {
		ad[i] = (int *) my_malloc(2*sizeof(int)); //8 bytes -> slot 0
		ad[i][0] = 99;
		ad[i][1] = 782;
		fflush(stdout);
	}

	printf("%d, %d --- %p\n", ad[0][0], ad[0][1], ad[0]);

	//printf("my_free()\n");
	//my_free((void *) ad2);

	// printf("\nCheck if freed elements are accessible.\n");

	return 0;
}