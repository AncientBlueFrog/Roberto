#include <math.h>
#include <stdio.h>
#include "ola.h"
#include "tchau.c"

int main(int argc, char *argv[])
{
	int a = 8;
	int b = 9;

	int c = pow(a, 2);

	printf("%d\n", c);
	say_hello(1);
	saybye();
	return 0;
}
