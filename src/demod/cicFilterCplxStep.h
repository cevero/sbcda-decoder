#ifndef CICFILTER_H
#define CICFILTER_H

#define maxVal 1073741824//=pow(2,30);
#define maxValDiv2 maxVal>>1

typedef struct {
  int *previousAccRe;
  int *previousAccIm;
  int accRe, accIm, accDlyIdx;
} mem_cic; 

void cicFilterCplxStep(int complex *inputSignal, mem_cic * str,int complex  *outputSignal, int decimation, int delay, int inputLength);

#endif
