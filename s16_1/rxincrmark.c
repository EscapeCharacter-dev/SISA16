#include <stdio.h>
#include <time.h>

volatile unsigned x;

int main(void){
	unsigned ms;
	x = 0;
	ms = clock()/(CLOCKS_PER_SEC/1000);
	while(
		ms ==  (
			clock()/(CLOCKS_PER_SEC/1000)
		)
	);
	do{
		x++;
	}while(x);
	ms = clock()/(CLOCKS_PER_SEC/1000) - ms;
	printf("milliseconds: %u",ms);
	return 0;
}
