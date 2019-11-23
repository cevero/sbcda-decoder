#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>

int main()
{
	char str[] = "     -265        -372.2          "; 
	char delim[] = " ";

    float complex z;
    float real, imag;

	char *ptr = strtok(str, delim);


    real = atof(ptr);
    ptr = strtok(NULL, delim);
    imag = atof(ptr);

    z = real+imag*I;
    printf("%.2f%+.2fi\n", creal(z), cimag(z));

	return 0;
}
