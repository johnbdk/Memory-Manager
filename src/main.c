#include "memory_management.h"

int main(int argc, char* argv[]){
	int *ad;
	int *ad2;

	mem_init();

	ad = (int *) my_malloc(2*sizeof(int));
	ad[0] = 99;
	ad[1] = 782;

	ad2 = (int *) my_malloc(16*sizeof(int));
	ad2[0] = 45;
	ad2[1] = 322;

	printf("%d, %d\n", ad2[0], ad2[1] );
	printf("%d, %d\n", ad[0], ad[1] );

	my_free((void *) ad2);
	my_free((void *) ad);

	ad = (int *) my_malloc(2*sizeof(int));
	ad[0] = 101;
	ad[1] = 888;
	printf("%d, %d\n", ad[0], ad[1] );

	return 0;
}