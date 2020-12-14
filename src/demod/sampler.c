#include "../service.h"
#include "sampler.h"
#include "cicFilterCplxStep.h"

//int complex lutHalfCycle[] = {127+0*I,117+49*I,90+90*I,49+117*I,0+127*I,-49+117*I,-90+90*I,-117+49*I};
int lutHalfCycleRe[] = {127,117,90,49,0,-49,-90,-117};
int lutHalfCycleIm[] = {0,49,90,117,127,117,90,49};
void sampler(int *demodSignal, sampler_mem * str_smp, mem_cic * str)
{
	int symbOut,idx;
	int i,i0,absMovAvg,offset,delayPlusOffset, integerDly, fractDly, delayDiff, delayUnWrap,x;
	int delay, delayFlt,angMovAvg;
	int freqShftSignalRe[] = {[0 ... nSymb-1]=0};
	int freqShftSignalIm[] = {[0 ... nSymb-1]=0};
//	int complex freqShftSignal[] = {[0 ... nSymb-1] = 0}, movingAvg, cicSignal;
	int movingAvgRe, movingAvgIm, cicSignalRe, cicSignalIm;
//	int complex sqrdSignal[] = {[0 ... nSymb-1] = 0};
	int sqrdSignalRe[] = {[0 ... nSymb-1]=0};
	int sqrdSignalIm[] = {[0 ... nSymb-1]=0};
	int accSymb;
  //apply frequency shift
	
	for(i = 0; i<nSymb; i++){		
		freqShftSignalRe[i] = (str_smp->lutSign*demodSignal[i]*lutHalfCycleRe[i])>>(LUTW-1);//freqShftSignal[i] = str_smp->lutSign*demodSignal[i]*lutHalfCycle[i]*pow(2,-(LUTW-1));
                freqShftSignalIm[i] = (str_smp->lutSign*demodSignal[i]*lutHalfCycleIm[i])>>(LUTW-1);
//              freqShftSignal[i] = floor(creal(freqShftSignal[i])) + floor(cimag(freqShftSignal[i]))*I;
		//square the signal
//		sqrdSignal[i] = creal(freqShftSignal[i])*creal(freqShftSignal[i])-cimag(freqShftSignal[i])*cimag(freqShftSignal[i])+creal(freqShftSignal[i])*cimag(freqShftSignal[i])*2I;
		sqrdSignalRe[i] = freqShftSignalRe[i]*freqShftSignalRe[i]-freqShftSignalIm[i]*freqShftSignalIm[i];
		sqrdSignalIm[i] = 2*freqShftSignalRe[i]*freqShftSignalIm[i];
	}
  str_smp->lutSign = -str_smp->lutSign;
	//Moving average
	cicFilterCplxStep(sqrdSignalRe, sqrdSignalIm, str, &cicSignalRe, &cicSignalIm, smpDeciRate, smpDelayIdx, nSymb);
	movingAvgRe = cicSignalRe/(avgLen*nSymb);
	movingAvgIm = cicSignalIm/(avgLen*nSymb);
      
//     	movingAvg = floor(creal(movingAvg))+floor(cimag(movingAvg))*I;
	
	//Get polar format of complex movingAvg
	int complex aux = movingAvgRe+movingAvgIm*I;
	absMovAvg = cabsf(aux);
//	absMovAvg = movingAvgRe*movingAvgRe+movingAvgIm*movingAvgIm;
	angMovAvg = atan2(movingAvgIm,movingAvgRe)*pow(2,18)/PI;
  //symbLock histeresys comparator  
	if(str_smp->symbLock){
		str_smp->symbLock = absMovAvg>(thHoldLow);
	} else{
		str_smp->symbLock = absMovAvg>(thHoldHigh);
	}
	//Calcule time delay in sampler
	if(str_smp->symbLock){
    delayFlt = angMovAvg/pow(2,smpthetaW);//#define thetaW, tSymb
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
	for(i=0;i<2*nSymb;i++){
		if(i<nSymb){
			str_smp->smplBuffer[i] = str_smp->smplBuffer[i+nSymb];
		}else{
			str_smp->smplBuffer[i] = demodSignal[i-nSymb];
		}
	}
	offset = nSymb*tSmpl;
	delayPlusOffset = delayUnWrap+offset;
  integerDly = floor(delayPlusOffset/tSmpl);
	fractDly = delayPlusOffset-(integerDly*tSmpl);
	
	idx = integerDly;
	accSymb = floor((str_smp->smplBuffer[idx+1]-str_smp->smplBuffer[idx])*fractDly/tSmpl);
  
 	str_smp->symbOut = str_smp->smplBuffer[idx]+accSymb;
 }
