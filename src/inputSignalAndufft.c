#include <stdio.h>
#include <stdlib.h>

#include <stddef.h>
#include <complex.h>
#include "fft.h"
//#include "ift.h"
#define NUMBER_OF_SAMPLES 64000

int main(int argc, char *argv[]){
	int i0;
	FILE* filePointer = fopen("edc_m2s_tb.txt","r");

	if(filePointer == NULL){
		printf("Can't open file!\n");
		return 1;
	}
	char output[10];
	int real[NUMBER_OF_SAMPLES];
	int imag[NUMBER_OF_SAMPLES];

	for (i0=0;fgets(output,sizeof(output), filePointer) != NULL;i0++){
		sscanf(output,"%d %d",&real[i0], &imag[i0]);
		//printf("%02d: %d\n", i0, imag[i0]);
	}
	fclose(filePointer);

	printf("***-----------------------------------****\n");
	printf("Finish loading signal\n");
	//printf("%d\n", imag[64000]);
	
	/********************************************************************************
	*/

	size_t W = 1280; // Window process
	size_t ZP = 2048-1280; //Zero padding
	size_t N = 2048;
	int n;

	float complex vector[N];

	while(){

		for(n = 0; n < N; n++) {
			if(n<W){
				vector[n] = real[n]+imag[n]*I;
			}else{
				vector[n] = 0;
			}
		}

		fft(vector,N);
		detect_loop();
	}

/*	Debug propose

	printf("in time domain:\n");

	for(n = 0; n < N; n++) {
		printf("%f%+fi\n", creal(vector[n]), cimag(vector[n]));
	}

	printf("in frequency domain:\n");

	for(n = 0; n < N; n++) {
		printf("%f%+fi\n", creal(vector[n]), cimag(vector[n]));
	}

	ift(vector, N);

	printf("in time domain:\n");

	for( n = 0; n < N; n++) {
		printf("%f%+fi\n", creal(vector[n]), cimag(vector[n]));
	}
*/
	return 0;
}