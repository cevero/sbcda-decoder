#ifndef DETECT_LOOP_H
#define DETECT_LOOP_H

#include <complex.h>

#define CONVERT_TO_32BIT_SIGNED(val, nbits)	(int) (val | (0xFFFFFFFF<<(nbits-1)))
#define TEST_SIGN_BIT(val, nbits)				((val & (0x00000001<<(nbits-1)))!=0)
#define DDS_FREQ_NUMBER_OF_BITS									11
#define DDS_NUMBER_OF_DECODERS									12
#define DDS_INSERT_FREQ_NEW 1
#define N_SAMPLE (2048)


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
	unsigned int passIdx[N_SAMPLE/DDS_NUMBER_OF_DECODERS];
	unsigned int passAmp[N_SAMPLE/DDS_NUMBER_OF_DECODERS];
} DDS_PassSet_Typedef;

typedef enum {
	DDS_INSERT_FREQ_NONE,
	FREQ_DETECTED_ONCE,
	FREQ_DETECTED_TWICE,
	FREQ_DECODING
} DDS_FreqsState_Typedef;

void calc_mask(int*mask,DDS_FreqsRecord_Typedef DDS_PTT_DP_LIST[DDS_NUMBER_OF_DECODERS]);
unsigned int DDS_Detection_Loop(float complex*signal);

#endif
