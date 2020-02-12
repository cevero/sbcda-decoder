#include "service.h"
#include "sampler.h"
#include "cicFilterCplxStep.h"

void cicFilterCplxStep(int complex *inputSignal, mem * str, float complex  *outputSignal)
{
  // Generare the CIC filter output for an input signal window 1280 samples
  // inputSignal: a complex signal with k*deciRate samples.
  // str: a structure with cicFitler state
  // outputSignal: a complex signal with k samples

  uint16_t iSmplOut = 0;
  uint16_t deciCount = 0;
  int previousAccRe[delayIdx], previousAccIm[delayIdx], accRe, accIm, acc, iSmpl,diffRe,diffIm, i; 
  uint8_t accDlyIdx;

	
  // read state load data in previous step
  for(i=0;i<delayIdx;i++){
    previousAccRe[i] = str->previousAccRe[i];
    previousAccIm[i] = str->previousAccIm[i];
  }
	
  accRe = str->accRe;
  accIm = str->accIm;
  accDlyIdx = str->accDlyIdx;
	
  for (iSmpl=0;iSmpl<inputSeqW;iSmpl++){
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
    if (deciCount==deciRate){
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
      outputSignal[iSmplOut] = (double) diffRe + (double) diffIm*I;      
      // update buffer index
      if (accDlyIdx>=delayIdx-1){
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
  for(i=0;i<delayIdx;i++){
    str->previousAccRe[i] = previousAccRe[i];
    str->previousAccIm[i] = previousAccIm[i];
  }
  str->accDlyIdx = accDlyIdx;
}
