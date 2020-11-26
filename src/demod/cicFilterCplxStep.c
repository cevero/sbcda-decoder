#include "../service.h"
#include "cicFilterCplxStep.h"

void cicFilterCplxStep(float complex *inputSignal, mem_cic * str,float complex  *outputSignal, int decimation, int delay, int inputLength)
{
  // Generare the CIC filter output for an input signal window 1280 samples
  // inputSignal: a complex signal with k*decimation samples.
  // str: a structure with cicFitler state
  // outputSignal: a complex signal with k samples

  int iSmplOut = 0;
  int deciCount = 0;
  int * previousAccRe = rt_alloc(RT_ALLOC_FC_RET_DATA,delay*sizeof(int));
  int * previousAccIm = rt_alloc(RT_ALLOC_FC_RET_DATA,delay*sizeof(int));
  int accDlyIdx, accRe, accIm, acc, iSmpl,diffRe,diffIm, i; 
;

	
  // read state load data in previous step
  for(i=0;i<delay;i++){
    previousAccRe[i] = str->previousAccRe[i];
    previousAccIm[i] = str->previousAccIm[i];
  }

  accRe = str->accRe;
  accIm = str->accIm;
  accDlyIdx = str->accDlyIdx;
	
  for (iSmpl=0;iSmpl<inputLength;iSmpl++){
    // acumulate with overflow  
    accRe = accRe + creal(inputSignal[iSmpl]);    
    if (accRe >= maxValDiv2){
      accRe = accRe - maxVal;
    }else if (accRe < -maxValDiv2){
      accRe = accRe + maxVal;
    }
    
    // acumulate with overflow
    accIm = accIm + cimag(inputSignal[iSmpl]);
    if (accIm >= maxValDiv2){
      accIm = accIm - maxVal;
    }else if (accIm < -maxValDiv2){      
      accIm = accIm + maxVal;
    }
    // decimation
    deciCount++;
    if (deciCount==decimation){
      deciCount = 0;      
      diffRe = accRe-previousAccRe[accDlyIdx];
      // overflow
      if (diffRe < (-1*maxValDiv2)){        
        diffRe = diffRe+maxVal;
      }else if (diffRe >=maxValDiv2){
        diffRe = diffRe-maxVal;
      }       
      // update buffer with previous decimator output
      previousAccRe[accDlyIdx] = accRe;    
      diffIm = accIm-previousAccIm[accDlyIdx];
      // overflow
      if (diffIm < -maxValDiv2){       
        diffIm = diffIm+maxVal;
      }else if (diffIm >=maxValDiv2){
        diffIm = diffIm-maxVal;
      }    
      // update buffer with previous decimator output
      previousAccIm[accDlyIdx] = accIm;    
      outputSignal[iSmplOut] = (double)diffRe-(double)diffIm*I;
      // update buffer index
      if (accDlyIdx>=delay-1){
        accDlyIdx = 0;
      }else{
        accDlyIdx++;
      }      
      iSmplOut++;
    }
  }

  // update state
  str->accRe = accRe;
  str->accIm = accIm;
  for(i=0;i<delay;i++){
    str->previousAccRe[i] = previousAccRe[i];
    str->previousAccIm[i] = previousAccIm[i];
  }
  str->accDlyIdx = accDlyIdx;
  rt_free(RT_ALLOC_FC_RET_DATA,previousAccRe,delay*sizeof(int));
  rt_free(RT_ALLOC_FC_RET_DATA,previousAccIm,delay*sizeof(int));
}
