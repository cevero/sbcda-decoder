// sampler
#define thHoldLow	256
#define thHoldHigh	4096
#define thetaW	19
#define tSymb	256 // symbol period in multiple of Tsymb/2^8
#define tSmpl	32 // sample period in multiple of Tsymb/2^8
#define smplPerSymb 8
#define avgLen 32 // error estimator average length in symbol
#define tHfSymb 128	// half symbol period in multiple of Tsymb/2^(8-1)
#define LUTW 8
#define inputSeqW 8
#define PI 3.14159265

#define N_SAMPLE 8
#define deciRate 8
#define delayIdx  32
#define SAMPLER	1

typedef struct 
{
  int previousAccRe[delayIdx];
  int previousAccIm[delayIdx];
  int accRe, accIm, accDlyIdx;
} mem; 

typedef struct 
{
  int smplBuffer [2*smplPerSymb];
  int delayAcc, prevDelay, lutSign, symbLock;
} sampler_mem;


int sampler (int *demodSignal, sampler_mem * str, mem * str_cic);