#include "streamflow.h"

int main(int argc, char* argv[]){
	// int *ad;
	// int *ad2;

	// mem_init();

	// ad = (int *) my_malloc(2*sizeof(int)); //8 bytes -> slot 0
	// ad[0] = 99;
	// ad[1] = 782;

	// ad2 = (int *) my_malloc(16*sizeof(int));  //64 bytes -> slot 3
	// ad2[0] = 45;
	// ad2[1] = 322;

	// printf("%d, %d\n", ad2[0], ad2[1] );
	// printf("%d, %d\n", ad[0], ad[1] );

	// my_free((void *) ad2);
	// my_free((void *) ad);

	// ad = (int *) my_malloc(2*sizeof(int)); //8 bytes -> slot 0
	// ad[0] = 101;
	// ad[1] = 888;
	// printf("%d, %d\n", ad[0], ad[1] );

	// printf("\nCheck sizes that is not power of 2.\n");
	// char *str = (char*) my_malloc(57*sizeof(char)); //57 bytes -> slot 3
	// memcpy(str, "boo", 3);

	// printf("%s\n", str);

	// my_free((void *) ad);
	// my_free((void *) str);

	// printf("\nCheck if freed elements are accessible.\n");
	// printf("%d, %d\n", ad[0], ad[1] );

	return 0;
}