#define symbRate 800
#define deciRate 20
#define delayIdx 8
#define delaySmp 32
#define deciRateSmp 8
#define coefW 8
#define freqW 20
#define thetaW 11
#define ncoAmpW 12
#define agcW 10
#define cordicW 14
#define smplRate 128000
#define vgaMantW 8
#define kpUInt 136
#define kpExp 17
#define kiUInt 182
#define kiExp 22
#define nSymb 8
#define smplPerSymb 160


#include "service.h"
#include "sampler.h"
#include "cicFilterCplxStep.h"


typedef struct {
	int lfAcc, thetaNco, ncoDFreq, symbCount;
	int * symbOut;
	int * symbLock;
} demod_mem;

void pttA2Demod(int complex * inputSignal,int ncoInitFreq,int vgaMant,int vgaExp, demod_mem * p, mem_cic * str, mem_cic * str1, sampler_mem * str_smp);