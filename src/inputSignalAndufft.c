#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <complex.h>
#include "fft.h"
#include "ift.h"
#include "detect_loop.h"

#define NUMBER_OF_SAMPLES 192000

int main(int argc, char *argv[]){
    if (!argv[1]) {
		printf("No file to process!\n");
        return 1;
    }
	FILE* filePointer = fopen(argv[1],"r");

	if(filePointer == NULL){
		printf("Can't open file!\n");
		return 1;
	}

	int i0;
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

	float complex *vector = malloc(sizeof(float complex)*N_SAMPLE);
    unsigned int *prevIdx = malloc(sizeof(unsigned int)*N_SAMPLE);
    unsigned int *nPrevIdx = malloc(sizeof(unsigned int));
    *nPrevIdx = 0;

    DDS_FreqsRecord_Typedef *DDS_PTT_DP_LIST = malloc(sizeof(DDS_FreqsRecord_Typedef)*DDS_NUMBER_OF_DECODERS);
    memset(prevIdx,0,N_SAMPLE);

    for (int i = 0; i < DDS_NUMBER_OF_DECODERS; i++) {
        DDS_PTT_DP_LIST[i].freq_idx = 0;
        DDS_PTT_DP_LIST[i].freq_amp = 0;
        DDS_PTT_DP_LIST[i].detect_state = DDS_INSERT_FREQ_NONE;
    }
    
    int count_offset = 0;
	int n,cnt=0;

    while(count_offset < NUMBER_OF_SAMPLES){
        for (n = 0; n < N_SAMPLE; n++) {
            if (n<W) {
                vector[n] = real[n+count_offset]+imag[n+count_offset]*I;
            } else {
                vector[n] = 0;
            }
        }

        printf("ret: %d cnt: %d\n",
                DDS_Detection_Loop(vector, prevIdx, nPrevIdx, DDS_PTT_DP_LIST), cnt);

        for (int i = 0; i < 3; i++) {
           printf("freq_idx: %d\n",DDS_PTT_DP_LIST[i].freq_idx);
        }

        count_offset += W;
        cnt++;
       // if (cnt==400) break;
    }
    for (int i = 0; i < DDS_NUMBER_OF_DECODERS; i++) {
        printf("\ni = %d, freq_idx = %d, amplitude = %d\n",i,DDS_PTT_DP_LIST[i].freq_idx,DDS_PTT_DP_LIST[i].freq_amp);
        
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

    free(vector);
    free(DDS_PTT_DP_LIST);
	return 0;
}
