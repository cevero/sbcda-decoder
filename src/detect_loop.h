#ifndef DETECT_LOOP_H
#define DETECT_LOOP_H
#include "../lib/kiss_fft.h"
#include <complex.h>
#include "service.h"
#define CONVERT_TO_32BIT_SIGNED(val, nbits)	\
                                    (int) (val | (0xFFFFFFFF<<(nbits-1)))
#define TEST_SIGN_BIT(val, nbits)	((val & (0x00000001<<(nbits-1)))!=0)
#define FREQ_NUMBER_OF_BITS	11
#define INSERT_FREQ_NEW 	1
#define FREQ_INVALID		-1
#define N_SAMPLE 		(8192)
#define DFT_LENGTH 		(2048)
#define LOG2DFT_LENGTH 		(11)
#define DEFAULT_AMP_THRESHOLD	(100)
#define DEFAULT_TIMEOUT 	(101)
#define WINDOW_LENGTH 		(1280)
#ifndef NoD
#define NoD 12
#endif
/*	DDS_FreqsRecord_Typedef will be passed to the next processing
*/

typedef struct {
	unsigned int freq_idx;
	unsigned int freq_amp;
	unsigned int detect_state;
	unsigned int timeout;
} FreqsRecord_T;

/*
*	DDS_PassSet_Typedef assists in this processing
*/
typedef struct {
	unsigned int Idx[32];
	unsigned int Amp[32];
} PassSet_T;

typedef enum {
	FREQ_NONE,//0
	FREQ_DETECTED_TWICE,//1
	FREQ_DECODING//2
} FreqsState_T;

void calc_mask(int * mask,FreqsRecord_T * PTT_DP_LIST[NoD]);

//unsigned int detectLoop(int complex *inputSignal,int * prevIdx, FreqsRecord_T * PTT_DP_LIST[NoD]);

unsigned int detectLoop(int complex * inputSignal, int * prevIdx, FreqsRecord_T * PTT_DP_LIST[NoD]);
#endif
