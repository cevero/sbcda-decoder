#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <complex.h>
#include "../lib/ufft/fft.h"
#define CONVERT_TO_32BIT_SIGNED(val, nbits)		(int) (val | (0xFFFFFFFF<<(nbits-1)))
#define TEST_SIGN_BIT(val, nbits)				((val & (0x00000001<<(nbits-1)))!=0)
#define DDS_FREQ_NUMBER_OF_BITS									11
#define DDS_NUMBER_OF_DECODERS									12
#define DDS_INSERT_FREQ_NEW 1
#define N 2048

/*	DDS_FreqsRecord_Typedef will be passed to the next processing
*/

typedef struct {
	unsigned int freq_idx;
	unsigned int freq_amp;
	unsigned int detect_state;
} DDS_FreqsRecord_Typedef;
/*
*	DDS_PassSet_Typedef assists in this processing
*/
typedef struct {
	unsigned int passIdx[N/DDS_NUMBER_OF_DECODERS];
	unsigned int passAmp[N/DDS_NUMBER_OF_DECODERS];
} DDS_PassSet_Typedef;

typedef enum {
	DDS_INSERT_FREQ_NONE,
	FREQ_DETECTED_ONCE,
	FREQ_DETECTED_TWICE,
	FREQ_DECODING
} DDS_FreqsState_Typedef;


