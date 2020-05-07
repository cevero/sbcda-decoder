#ifndef SAMPLER_H
#define SAMPLER_H

// sampler
#define thHoldLow	256
#define thHoldHigh	4096
#define smpthetaW	19
#define tSymb	256 // symbol period in multiple of Tsymb/2^8
#define tSmpl	32 // sample period in multiple of Tsymb/2^8
#define avgLen 32 // error estimator average length in symbol
#define tHfSymb 128	// half symbol period in multiple of Tsymb/2^(8-1)
#define LUTW 8
#define nSymb 8
#define PI 3.14159265
#define smpDeciRate 8
#define smpDelayIdx  32


typedef struct {
  int *smplBuffer;
  int delayAcc, prevDelay, lutSign, symbLock, symbOut;
} sampler_mem;

#include "cicFilterCplxStep.h"
void sampler(int *demodSignal, sampler_mem * str, mem_cic * str_cic);

#endif