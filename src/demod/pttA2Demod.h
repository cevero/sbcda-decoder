#include "../service.h"
#include "sampler.h"
#include "cicFilterCplxStep.h"

#define symbRate (800) //symbRate of input
#define deciRate (20)//decirate of CIC filter
#define delayIdx (8)//delay of CIC filter
#define delaySmp (32)//delay of CIC filter in Sampler
#define deciRateSmp (8)//decirate of CIC filter in Sampler
#define coefW (8)
#define freqW (20)//number of bits of frequency (NCO)
#define thetaW (11)//number of bits of angle theta (Loop Filter)
#define ncoAmpW (12)//number of bits of nco signal
#define agcW (10)//number of bits of vga gain
#define cordicW (14)
#define smplRate (128000)//sample rate of input
#define vgaMantW (8)
#define kpUInt (136)//mantissa proportional gain of Loop Filter
#define kpExp (17)//exponential proportional gain of Loop Filter
#define kiUInt (182)//mantissa integrative gain of Loop Filter
#define kiExp (22)//exponential integrative gain of Loop Filter

#ifndef nSymb
#define nSymb (8)
#endif

#ifndef NoD
#define NoD (12)
#endif

#ifndef NoC
#define NoC (8)
#endif

#define smplPerSymb  (160)//samples per symbol rate


typedef struct {
	int lfAcc, thetaNco, ncoDFreq, symbCount;
	int * symbOut;
	int * symbLock;
} demod_mem;

typedef struct demodArg{
	int * inputBlockRe[NoC];
	int * inputBlockIm[NoC];
	int * ncoTheta[NoC];
	int * InitFreq;
	int * vgaMant;
	int * vgaExp;
	int iSymb;
	int activeList;
	short nSeq;
	PTTService_T * wpckg[NoD];
	demod_mem * str_demod[NoD];
	mem_cic * str_cic[NoD];
	mem_cic * str_cicSmp[NoD];
	sampler_mem * str_smp[NoD];
}demodArg_t;

int pttA2Demod(int complex * inputSignal,int ncoInitFreq,int vgaMant,int vgaExp, demod_mem * p, mem_cic * str, mem_cic * str1, sampler_mem * str_smp);
//
//int pttA2Demod(int complex * inputSignal, demodArg_t * ptr);


int numberActiveDecod(int activeList);
int prlpttA2Demod(int complex * inputSignal, demodArg_t * ptr);
int pttA2DemodStep(demodArg_t * ptr);
