#include <stdio.h>
#include <stdlib.h>

#include <stddef.h>
#include <complex.h>
#include "fft.h"
#include "ift.h"
#include "detect_loop.h"

#define NUMBER_OF_SAMPLES 64000

int main(int argc, char *argv[]){
	int i0;
	FILE* filePointer = fopen("../matlab/edc_m2s_tb.txt","r");

	if(filePointer == NULL){
		printf("Can't open file!\n");
		return 1;
	}

	char output[10];
	int real[NUMBER_OF_SAMPLES];
	int imag[NUMBER_OF_SAMPLES];

	for (i0=0;fgets(output,sizeof(output), filePointer) != NULL;i0++){
		sscanf(output,"%d %d",&real[i0], &imag[i0]);
	}

	fclose(filePointer);

	printf("***-----------------------------------****\n");
	printf("Finish loading signal\n");
	
	/************************************************************************/

	size_t W = 1280; // Window process
	/*size_t ZP = N_SAMPLE-1280; //Zero padding*/
    size_t N = 2048;
	int n;

	float complex vector[N_SAMPLE];

    int window_count = 0;
    int count_offset = 0;

    while(count_offset < NUMBER_OF_SAMPLES){
        for (n = 0; n < N_SAMPLE; n++) {
            if (n<W) {
                vector[n] = real[n+count_offset]+imag[n+count_offset]*I;
            } else {
                vector[n] = 0;
            }
        }

        printf("%d %d\n",window_count,DDS_Detection_Loop((float complex *) &vector));
        window_count++;
        count_offset = W*window_count;
    }

/*	Debug propose */
#ifdef DEBUG

	printf("in time domain:\n");

	for(n = 0; n < N_SAMPLE; n++) {
		printf("%f+%fi\n", creal(vector[n]), cimag(vector[n]));
	}

	printf("in frequency domain:\n");
    float complex vector_time[N_SAMPLE];

    for (int i = 0; i < N_SAMPLE; i++) {
        vector_time[i] = vector[i]; 
    }
    
    fft(vector,N_SAMPLE);

	for(n = 0; n < N_SAMPLE; n++) {
        printf("%f+%fi\n", creal(vector[n]), cimag(vector[n]));
	}

	ift(vector, N_SAMPLE);

	printf("in time domain, after and before:\n");

	for( n = 0; n < N_SAMPLE; n++) {
        printf("%f+%fi,%f+%fi\n", creal(vector[n]), cimag(vector[n]), 
                           creal(vector_time[n]), cimag(vector_time[n]));
	}
#endif
	return 0;
}
