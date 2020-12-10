#include "pttA2Demod.h"
#include "ncoLut.h"
#include "stdlib.h"

void pttA2Demod(int complex * inputSignal,int ncoInitFreq,int vgaMant,int vgaExp, demod_mem * p, mem_cic * str, mem_cic * str1, sampler_mem * str_smp)
{
	int iSymb, i0;
	
	int complex * inputBlock = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int complex));
	int complex * ncoSignal = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int complex));
	int complex * cplxMult = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int complex));
	int complex * mfSignal = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int complex));
	int ncoFreq; 
	int * demodSignal = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int));
	int * ncoTheta = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
	int complex * vgaSignal = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int complex));

	int lutAddr;
	int complex deciSignal;
	int angDeciSignal, theta, piAdd,lpf;

	for(iSymb = 0; iSymb<nSymb;iSymb++){
		p->symbCount++;

		/*
		*	Partitioning of the input signal per symbol (160 samples/symbol)
		*/
		for(i0=0;i0<smplPerSymb;i0++){
			inputBlock[i0] = inputSignal[i0+smplPerSymb*iSymb];
		}

		/*
		*	NCO update and LUT
		*/
		ncoFreq = p->ncoDFreq+ncoInitFreq;
		
		incAndOvFlow(p->thetaNco, ncoFreq, smplPerSymb, freqW, ncoTheta);
		p->thetaNco = ncoTheta[smplPerSymb-1];
		
		for(i0=0;i0<smplPerSymb;i0++){			
			lutAddr = ncoTheta[i0] >> (int)cabs(thetaW-freqW);
			ncoSignal[i0] = creal(ncoLut[lutAddr])-cimag(ncoLut[lutAddr])*I;
		}		

		/*
		*	Complex Multiplier
		* floor(float)
		*/		

		for (i0=0;i0<smplPerSymb;i0++){
			cplxMult[i0] = inputBlock[i0]*ncoSignal[i0];
			cplxMult[i0] = ((int complex) cplxMult[i0])*pow(2,-(ncoAmpW-1));
			cplxMult[i0] = floor(creal(cplxMult[i0]))+floor(cimag(cplxMult[i0]))*I;
		}

		/*
		*	Matched filter and decimation
		*/
		cicFilterCplxStep(cplxMult,str,mfSignal,deciRate,delayIdx,smplPerSymb);

		/*
		*	VGA input mfSignal output demodSignal
		*/
		for(i0=0;i0<smplPerSymb/deciRate;i0++){
			vgaSignal[i0] = floor((creal(mfSignal[i0])*vgaMant)*pow(2,vgaExp-vgaMantW))+floor((cimag(mfSignal[i0])*vgaMant)*pow(2,vgaExp-vgaMantW))*I;
			demodSignal[i0] = cimag(vgaSignal[i0]);
		}
		/*
		*	Timer recovering SAMPLER
		*/
		
		sampler(demodSignal, str_smp, str1);
		p->symbOut[iSymb] = str_smp->symbOut;
		p->symbLock[iSymb] = str_smp->symbLock;

		/*
		*	CIC2
		*/
		deciSignal = 0+0*I;
		for (i0 = 0;i0<deciRateSmp;i0++){
			deciSignal = deciSignal+vgaSignal[i0];
		}
		//absDeciSignal = cabsf(deciSignal);
		angDeciSignal = atan2(cimag(deciSignal),creal(deciSignal))*pow(2,cordicW-1)/PI;
		theta = angDeciSignal*pow(2,(thetaW-cordicW));
		/*
		*	PLL Loop Filter
		*/

		piAdd = kpUInt*theta+(p->lfAcc>>((int)cabs(kpExp-kiExp)));		
		p->lfAcc +=kiUInt*theta;
		lpf = piAdd>>((int)cabs(freqW-thetaW-kpExp));
		p->ncoDFreq = lpf;
	}
rt_free(MEM_ALLOC,inputBlock,smplPerSymb*(sizeof(int complex)));
rt_free(MEM_ALLOC,ncoSignal,smplPerSymb*(sizeof(int complex)));
rt_free(MEM_ALLOC,vgaSignal,(smplPerSymb/deciRate)*sizeof(int complex));
rt_free(MEM_ALLOC,cplxMult,smplPerSymb*(sizeof(int complex)));   
rt_free(MEM_ALLOC,mfSignal,(smplPerSymb/deciRate)*(sizeof(int complex)));
rt_free(MEM_ALLOC,demodSignal,(smplPerSymb/deciRate)*sizeof(int));
rt_free(MEM_ALLOC,ncoTheta,(smplPerSymb)*sizeof(int));
}






