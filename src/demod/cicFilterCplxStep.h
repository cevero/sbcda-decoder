
#ifndef SAMPLER
	#define N_SAMPLE 1280 //extern
	#define inputSeqW 160
	#define deciRate 20 //extern
	#define delayIdx  8 //extern
	#define smplPerSymb 8
	typedef struct 
	{
  	int previousAccRe[delayIdx];
  	int previousAccIm[delayIdx];
  	int accRe, accIm, accDlyIdx;
  	int delayAcc, prevDelay;
	} mem;
#endif

#define length N_SAMPLE/deciRate
#define maxVal 1073741824//=pow(2,30);
#define maxValDiv2 maxVal>>1



void cicFilterCplxStep(int complex *inputSignal, mem * str, float complex  *outputSignal);