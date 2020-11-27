#ifndef DETECT_LOOP_H
#define DETECT_LOOP_H
#include "../lib/kiss_fft.h"
#include <complex.h>
#include "service.h"
#define CONVERT_TO_32BIT_SIGNED(val, nbits)	\
                                    (int) (val | (0xFFFFFFFF<<(nbits-1)))
#define TEST_SIGN_BIT(val, nbits)	((val & (0x00000001<<(nbits-1)))!=0)
#define FREQ_NUMBER_OF_BITS									11
#define NUMBER_OF_DECODERS									12
#define INSERT_FREQ_NEW 1
#define FREQ_INVALID	-1
#define N_SAMPLE (8192)
#define DFT_LENGTH (2048)
#define DEFAULT_AMP_THRESHOLD (500)
#define DEFAULT_TIMEOUT (101)
#define WINDOW_LENGTH (1280)
/*	DDS_FreqsRecord_Typedef will be passed to the next processing
*/

typedef struct {
	unsigned int freq_idx;
	unsigned int freq_amp;
	unsigned int detect_state;
	unsigned int timeout;
} FreqsRecord_Typedef;

/*
*	DDS_PassSet_Typedef assists in this processing
*/
typedef struct {
	unsigned int Idx[32];
	unsigned int Amp[32];
} PassSet_Typedef;

typedef enum {
	FREQ_NONE,//0
	FREQ_DETECTED_TWICE,//1
	FREQ_DECODING//2
} FreqsState_Typedef;

void calc_mask(int * mask,FreqsRecord_Typedef * PTT_DP_LIST[NUMBER_OF_DECODERS]);

//unsigned int detectLoop(int complex *inputSignal,int * prevIdx, FreqsRecord_Typedef * PTT_DP_LIST[NUMBER_OF_DECODERS]);

unsigned int detectLoop(int complex *inputSignal, int * prevIdx, FreqsRecord_Typedef * PTT_DP_LIST[NUMBER_OF_DECODERS]);
#endif
