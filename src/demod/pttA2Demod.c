#include "pttA2Demod.h"
#include "ncoLut.h"
#include "stdlib.h"

void pttA2Demod(int complex * inputSignal,int ncoInitFreq,int vgaMant,int vgaExp, demod_mem * p, mem_cic * str, mem_cic * str1, sampler_mem * str_smp)
{
	int iSymb, i0;
	
//	int complex * inputBlock = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int complex));
	int * inputBlockRe = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));	
	int * inputBlockIm = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
//	int complex * ncoSignal = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int complex));
        int * ncoSignalRe = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
        int * ncoSignalIm = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
//	int complex * cplxMult = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int complex));
	int * cplxMultRe = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
	int * cplxMultIm = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
//	int complex * mfSignal = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int complex));
	int * mfSignalRe = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int));
	int * mfSignalIm = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int));

	int * demodSignal = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int));
	int * ncoTheta = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
//	int complex * vgaSignal = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int complex));
	int * vgaSignalRe = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int));
	int * vgaSignalIm = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int));

	int ncoFreq; 
	int lutAddr;
	int deciSignalRe, deciSignalIm;
	int angDeciSignal, theta, piAdd,lpf;
	int cic_time, sampler_time,cpxMult_time;

	for(iSymb = 0; iSymb<nSymb;iSymb++){
		p->symbCount++;

		/*
		*	Partitioning of the input signal per symbol (160 samples/symbol)
		*/
		for(i0=0;i0<smplPerSymb;i0++){
			inputBlockRe[i0] = creal(inputSignal[i0+smplPerSymb*iSymb]);
			inputBlockIm[i0] = cimag(inputSignal[i0+smplPerSymb*iSymb]);
		}

		/*
		*	NCO update and LUT
		*/
		ncoFreq = p->ncoDFreq+ncoInitFreq;
		
		incAndOvFlow(p->thetaNco, ncoFreq, smplPerSymb, freqW, ncoTheta);
		p->thetaNco = ncoTheta[smplPerSymb-1];
		
		for(i0=0;i0<smplPerSymb;i0++){			
			lutAddr = ncoTheta[i0] >> (int)cabs(thetaW-freqW);
			ncoSignalRe[i0] = creal(ncoLut[lutAddr]);
			ncoSignalIm[i0] = -1*cimag(ncoLut[lutAddr]);
		}		

		/*
		*	Complex Multiplier
		* floor(float)
		*/		
		cpxMult_time = rt_time_get_us();

		for (i0=0;i0<smplPerSymb;i0++){
			cplxMultRe[i0] = (inputBlockRe[i0]*ncoSignalRe[i0]-inputBlockIm[i0]*ncoSignalIm[i0])>>(ncoAmpW-1);//cplxMult[i0] = inputBlock[i0]*ncoSignal[i0];
			cplxMultIm[i0] = (inputBlockRe[i0]*ncoSignalIm[i0]+inputBlockIm[i0]*ncoSignalRe[i0])>>(ncoAmpW-1);
//			((int complex) cplxMult[i0])*pow(2,-(ncoAmpW-1));
//			cplxMult[i0] = floor(creal(cplxMult[i0]))+floor(cimag(cplxMult[i0]))*I;
		}
		cpxMult_time = rt_time_get_us()-cpxMult_time;
		if(iSymb==0){
//			printf("cpxMult time: %d us\n",cpxMult_time);
		}
		/*
		*	Matched filter and decimation
		*/

                cic_time = rt_time_get_us();
		cicFilterCplxStep(cplxMultRe,cplxMultIm,str,mfSignalRe,mfSignalIm,deciRate,delayIdx,smplPerSymb);
		cic_time = rt_time_get_us()-cic_time;
		if(iSymb==0){
//			printf("cic time: %d us\n",cic_time);
		}
		/*
		*	VGA input mfSignal output demodSignal
		*/
		for(i0=0;i0<smplPerSymb/deciRate;i0++){
//			vgaSignal[i0] = floor((creal(mfSIGNAL)*vgaMant)*pow(2,vgaExp-vgaMantW))+floor((cimag(mfSIGNAL)*vgaMant)*pow(2,vgaExp-vgaMantW))*I;
			vgaSignalRe[i0] = floor( mfSignalRe[i0]*vgaMant>>((-1*vgaExp)+vgaMantW));
			vgaSignalIm[i0] = floor( mfSignalIm[i0]*vgaMant>>((-1*vgaExp)+vgaMantW));
			demodSignal[i0] = vgaSignalIm[i0];
		}
		/*
		*	Timer recovering SAMPLER
		*/
		sampler_time = rt_time_get_us();		
		sampler(demodSignal, str_smp, str1);
		sampler_time = rt_time_get_us()-sampler_time;
		if(iSymb==0){
//			printf("sampler time: %d us\n",sampler_time);
		}

		p->symbOut[iSymb] = str_smp->symbOut;
		p->symbLock[iSymb] = str_smp->symbLock;

		/*
		*	CIC2
		*/
		deciSignalRe = 0;deciSignalIm = 0;
		for (i0 = 0;i0<deciRateSmp;i0++){
			deciSignalRe = deciSignalRe+vgaSignalRe[i0];
			deciSignalIm = deciSignalIm+vgaSignalIm[i0];
		}
		//absDeciSignal = cabsf(deciSignal);
		angDeciSignal = atan2(deciSignalIm,deciSignalRe)*pow(2,cordicW-1)/PI;
		theta = angDeciSignal>>(cordicW-thetaW);
		/*
		*	PLL Loop Filter
		*/
		piAdd = kpUInt*theta+(p->lfAcc>>((int)cabs(kpExp-kiExp)));		
		p->lfAcc +=kiUInt*theta;
		lpf = piAdd>>((int)cabs(freqW-thetaW-kpExp));
		p->ncoDFreq = lpf;	

	}
//rt_free(MEM_ALLOC,inputBlock,smplPerSymb*(sizeof(int complex)));
rt_free(MEM_ALLOC,inputBlockRe,smplPerSymb*(sizeof(int)));
rt_free(MEM_ALLOC,inputBlockIm,smplPerSymb*(sizeof(int)));
//rt_free(MEM_ALLOC,ncoSignal,smplPerSymb*(sizeof(int complex)));
rt_free(MEM_ALLOC,ncoSignalRe,smplPerSymb*(sizeof(int)));
rt_free(MEM_ALLOC,ncoSignalIm,smplPerSymb*(sizeof(int)));
//rt_free(MEM_ALLOC,vgaSignal,(smplPerSymb/deciRate)*sizeof(int complex));
rt_free(MEM_ALLOC,vgaSignalRe,(smplPerSymb/deciRate)*sizeof(int));
rt_free(MEM_ALLOC,vgaSignalIm,(smplPerSymb/deciRate)*sizeof(int));
//rt_free(MEM_ALLOC,cplxMult,smplPerSymb*(sizeof(int complex)));   
rt_free(MEM_ALLOC,cplxMultRe,smplPerSymb*(sizeof(int)));   
rt_free(MEM_ALLOC,cplxMultIm,smplPerSymb*(sizeof(int)));   
//rt_free(MEM_ALLOC,mfSignal,(smplPerSymb/deciRate)*(sizeof(int complex)));
rt_free(MEM_ALLOC,mfSignalRe,(smplPerSymb/deciRate)*(sizeof(int)));
rt_free(MEM_ALLOC,mfSignalIm,(smplPerSymb/deciRate)*(sizeof(int)));
rt_free(MEM_ALLOC,demodSignal,(smplPerSymb/deciRate)*sizeof(int));
rt_free(MEM_ALLOC,ncoTheta,(smplPerSymb)*sizeof(int));
}






