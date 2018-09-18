
#include <stdio.h>
#include <stdlib.h>

int cmnMuxDsValidate(void);

int main(int argc, char **argv)
{
	if( cmnMuxDsValidate() )
	{
		printf("MAC verification failed!\n");
		return 1;
	}

	printf("MAC verification OK!\n");
	return 0;
}

