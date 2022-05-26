#include <stdio.h>

int main(void)
{
	FILE *fb = fopen("./Makefile", "w+");

	if(!fb)
	{
		printf("Danou-se.");
		return 1;
	}

	fclose(fb);

	return 0;
}
