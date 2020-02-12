#include "service.h"
#include "sampler.h"
#include "cicFilterCplxStep.h"
//int symbLock;
int complex lutHalfCycle[] = {127+0*I,117+49*I,90+90*I,49+117*I,
								0+127*I,-49+117*I,-90+90*I,-117+49*I};
int sampler (int *demodSignal, sampler_mem * str_smp, mem * str)
{
	int symbOut;
	int i,i0,absMovAvg,offset,delayPlusOffset, integerDly, fractDly, delayDiff, delayUnWrap, idx;
	float delay, delayFlt,angMovAvg;
	float complex freqShftSignal[] = {[0 ... smplPerSymb-1] = 0}, movingAvg, cicSignal;
	int complex sqrdSignal[] = {[0 ... smplPerSymb-1] = 0};
	
  //apply frequency shift
	for(i = 0; i<inputSeqW; i++){
		freqShftSignal[i] = str_smp->lutSign*demodSignal[i]*lutHalfCycle[i]*pow(2,-(LUTW-1));
    freqShftSignal[i] = floor(creal(freqShftSignal[i])) + floor(cimag(freqShftSignal[i]))*I;
		
		//square the signal
		sqrdSignal[i] = creal(freqShftSignal[i])*creal(freqShftSignal[i])-cimag(freqShftSignal[i])*cimag(freqShftSignal[i])+creal(freqShftSignal[i])*cimag(freqShftSignal[i])*2I;    
	}
  str_smp->lutSign = -str_smp->lutSign;
	
	//Moving average
	cicFilterCplxStep(sqrdSignal, str, &cicSignal);
	
	movingAvg = cicSignal/(avgLen*smplPerSymb);
  movingAvg = floor(creal(movingAvg))+floor(cimag(movingAvg))*I;
	//Get polar format of complex movingAvg
	absMovAvg = cabsf(movingAvg);
	angMovAvg = atan2(cimag(movingAvg),creal(movingAvg))*pow(2,18)/PI;
  //symbLock histeresys comparator  
	if(str_smp->symbLock){
		str_smp->symbLock = absMovAvg>thHoldLow;
	} else{
		str_smp->symbLock = absMovAvg>thHoldHigh;
	}
	//Calcule time delay in sampler
	if(str_smp->symbLock){
    delayFlt = angMovAvg/pow(2,thetaW);//#define thetaW, tSymb
    delay = floor(delayFlt*tSymb+0.5); //float delay, delayFlt
    delayDiff = delay-str_smp->prevDelay;
    str_smp->prevDelay = delay;
		if(delayDiff>=tHfSymb){
			delayDiff = delayDiff-tSymb;
		}else if(delayDiff<-tHfSymb){
			delayDiff = delayDiff+tSymb;
		}
		str_smp->delayAcc +=delayDiff;
		delayUnWrap = str_smp->delayAcc;    
	}else{
		delay = 0;
		delayUnWrap = 0;
	}
  
	if(delayUnWrap >= 0.75*tSymb){
		delayUnWrap = 0.75*tSymb-1;
	} else if (delayUnWrap < -0.75*tSymb){
		delayUnWrap= -0.75*tSymb;
	}
  
	//Initializes smplBuffer with zeros;
	for(i=0;i<2*smplPerSymb;i++){
		if(i<smplPerSymb){
			str_smp->smplBuffer[i] = str_smp->smplBuffer[i+smplPerSymb];
		}else{
			str_smp->smplBuffer[i] = demodSignal[i-smplPerSymb];
		}
    //printf("smplBuffer[%d]: %d \n",i, str_smp->smplBuffer[i]);
	}
  
	offset = smplPerSymb*tSmpl;
	delayPlusOffset = delayUnWrap+offset;

  integerDly = floor(delayPlusOffset/tSmpl);
	fractDly = delayPlusOffset-(integerDly*tSmpl);
  
	idx = integerDly;
  float accSymb = floor((str_smp->smplBuffer[idx+1]-str_smp->smplBuffer[idx])*fractDly/tSmpl);
  
	symbOut = str_smp->smplBuffer[idx]+accSymb;
  //printf("[%d] %d\n", str_smp->symbLock, symbOut);
	return symbOut;
}