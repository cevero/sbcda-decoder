#include "pttA2Demod.h"
#include "ncoLut.h"
#include "stdlib.h"

int pttA2DemodStep(demodArg_t * ptr)
{

	int iSymb, i0;
	int mfSignalRe[smplPerSymb/deciRate];
	int mfSignalIm[smplPerSymb/deciRate];
	int vgaSignalRe[smplPerSymb/deciRate]; 
	int vgaSignalIm[smplPerSymb/deciRate];
	int demodSignal[smplPerSymb/deciRate];

	int ncoFreq; 
	int lutAddr;
	int deciSignalRe, deciSignalIm;
	int angDeciSignal, theta, piAdd,lpf;
	int cic_time, sampler_time,cpxMult_time;
	int auxRe,auxIm;
	/***************************************************************************/
	int * inputBlockRe;
	inputBlockRe = ptr->inputBlockRe[rt_core_id()];
	int * inputBlockIm;
	inputBlockIm = ptr->inputBlockIm[rt_core_id()];
	int * ncoTheta;
	ncoTheta = ptr->ncoTheta[rt_core_id()];

	int ncoInitFreq = ptr->InitFreq[rt_core_id()];
	int vgaMant = ptr->vgaMant[rt_core_id()];
	int vgaExp = ptr->vgaExp[rt_core_id()];

	demod_mem * p;
	p = (ptr->str_demod[rt_core_id()]);
	mem_cic * str;
	str = (ptr->str_cic[rt_core_id()]);
	mem_cic * str1;
	str1 = (ptr->str_cicSmp[rt_core_id()]);
	sampler_mem * str_smp;
	str_smp = (ptr->str_smp[rt_core_id()]);
	/***************************************************************************/
//	if(rt_core_id()==0)
//		printf("(%d) %d\n",rt_core_id(),inputBlockRe[0]);
	iSymb = ptr->iSymb;
	p->symbCount++;
	/*
	*	NCO update and LUT
	*/

	ncoFreq = p->ncoDFreq+ncoInitFreq;

	incAndOvFlow(p->thetaNco, ncoFreq, smplPerSymb, freqW, ncoTheta);
	p->thetaNco = ncoTheta[smplPerSymb-1];
       
		
	/*
	*	Complex Multiplier
	* 	floor(float)
	*/		
	for (i0=0;i0<smplPerSymb;i0++){
//		cplxMult[i0] = inputBlock[i0]*ncoSignal[i0];
		lutAddr = ncoTheta[i0] >> (int) (freqW-thetaW);
		auxRe = (inputBlockRe[i0]*((int)creal(ncoLut[lutAddr]))-inputBlockIm[i0]*-1*((int)cimag(ncoLut[lutAddr])))>>(ncoAmpW-1);
		auxIm = (inputBlockRe[i0]*-1*((int)cimag(ncoLut[lutAddr]))+inputBlockIm[i0]*((int)creal(ncoLut[lutAddr])))>>(ncoAmpW-1);
		inputBlockRe[i0] = auxRe;
		inputBlockIm[i0] = auxIm;
//		((int complex) cplxMult[i0])*pow(2,-(ncoAmpW-1));
//		cplxMult[i0] = floor(creal(cplxMult[i0]))+floor(cimag(cplxMult[i0]))*I;
	}
	

	if(iSymb==0){
//		printf("cpxMult time: %d us\n",cpxMult_time);
	}
	/*
	*	Matched filter and decimation
	*/

	cicFilter(inputBlockRe,inputBlockIm,str,mfSignalRe,mfSignalIm,deciRate,delayIdx,smplPerSymb);
	

	/*
	*	VGA input mfSignal output demodSignal
	*/

	for(i0=0;i0<smplPerSymb/deciRate;i0++){
//		vgaSignal[i0] = floor((creal(mfSIGNAL)*vgaMant)*pow(2,vgaExp-vgaMantW))+floor((cimag(mfSIGNAL)*vgaMant)*pow(2,vgaExp-vgaMantW))*I;
		vgaSignalRe[i0] = floor( mfSignalRe[i0]*vgaMant>>((-1*vgaExp)+vgaMantW));
		vgaSignalIm[i0] = floor( mfSignalIm[i0]*vgaMant>>((-1*vgaExp)+vgaMantW));
		demodSignal[i0] = vgaSignalIm[i0];
	}

	/*
	*	Timer recovering SAMPLER
	*/
	sampler(demodSignal, str_smp, str1);
/*
	if(rt_core_id()==1){
		printf("(%d) %d\n",rt_core_id(),str_smp->symbOut);
	}
*/
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
	angDeciSignal = ((int)atan2(deciSignalIm,deciSignalRe)<<(cordicW-1))/PI;
	theta = angDeciSignal>>(cordicW-thetaW);
	/*
	*	PLL Loop Filter
	*/
	piAdd = kpUInt*theta+(p->lfAcc>>(kiExp-kpExp));		
	p->lfAcc +=kiUInt*theta;
	lpf = piAdd>>(kpExp+thetaW-freqW);
	p->ncoDFreq = lpf;	
	return 0;
}

int prlpttA2Demod(int complex * inputSignal, demodArg_t * ptr)
{

	int i0, iSymb;
	ptr->inputBlockRe[0] = rt_alloc(MEM_ALLOC, smplPerSymb*sizeof(int));
	ptr->inputBlockRe[1] = rt_alloc(MEM_ALLOC, smplPerSymb*sizeof(int));
	ptr->inputBlockIm[0] = rt_alloc(MEM_ALLOC, smplPerSymb*sizeof(int));
	ptr->inputBlockIm[1] = rt_alloc(MEM_ALLOC, smplPerSymb*sizeof(int));
	ptr->ncoTheta[0] = rt_alloc(MEM_ALLOC, smplPerSymb*sizeof(int));
	ptr->ncoTheta[1] = rt_alloc(MEM_ALLOC, smplPerSymb*sizeof(int));

	for(iSymb = 0; iSymb<nSymb;iSymb++){
		/*
		*	Partitioning of the input signal per symbol (160 samples/symbol)
		*/
		for(i0=0;i0<smplPerSymb;i0++){
			ptr->inputBlockRe[0][i0] = creal(inputSignal[i0+smplPerSymb*iSymb]);
			ptr->inputBlockIm[0][i0] = cimag(inputSignal[i0+smplPerSymb*iSymb]);
			ptr->inputBlockRe[1][i0] = creal(inputSignal[i0+smplPerSymb*iSymb]);
			ptr->inputBlockIm[1][i0] = cimag(inputSignal[i0+smplPerSymb*iSymb]);
		}

		ptr->iSymb = iSymb;
//		printf("%d %d \n",ptr->inputBlockRe[0][0],ptr->inputBlockIm[0][0]);
		rt_team_fork(2, (void *) pttA2DemodStep, (void *) ptr);
	}

	rt_free(MEM_ALLOC,ptr->inputBlockRe[0],smplPerSymb*(sizeof(int)));
	rt_free(MEM_ALLOC,ptr->inputBlockIm[0],smplPerSymb*(sizeof(int)));
	rt_free(MEM_ALLOC,ptr->inputBlockRe[1],smplPerSymb*(sizeof(int)));
	rt_free(MEM_ALLOC,ptr->inputBlockIm[1],smplPerSymb*(sizeof(int)));
	rt_free(MEM_ALLOC,ptr->ncoTheta[0],smplPerSymb*(sizeof(int)));
	rt_free(MEM_ALLOC,ptr->ncoTheta[1],smplPerSymb*(sizeof(int)));
	return 0;
}




//int pttA2Demod(int complex * inputSignal, demodArg_t * ptr)
int pttA2Demod(int complex * inputSignal,int ncoInitFreq,int vgaMant,int vgaExp, demod_mem * p, mem_cic * str, mem_cic * str1, sampler_mem * str_smp)
{

	
	int * inputBlockRe = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));	
	int * inputBlockIm = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
//	int * cplxMultRe = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
//	int * cplxMultIm = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
//	int * mfSignalRe = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int));
//	int * mfSignalIm = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int));
/*  	int inputBlockRe[160];
	int inputBlockIm[160];
	int ncoTheta[160]; 
	int ncoSignalRe[160];
	int ncoSignalIm[160];
	int cplxMultRe[160];
	int cplxMultIm[160];
*/
	int iSymb, i0;
	int mfSignalRe[smplPerSymb/deciRate];
	int mfSignalIm[smplPerSymb/deciRate];
	int vgaSignalRe[smplPerSymb/deciRate]; 
	int vgaSignalIm[smplPerSymb/deciRate];
	int demodSignal[smplPerSymb/deciRate];
	int ncoTheta[smplPerSymb];

//	int * demodSignal = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int));
//	int * ncoTheta = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
//	int * vgaSignalRe = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int));
//	int * vgaSignalIm = rt_alloc(MEM_ALLOC,(smplPerSymb/deciRate)*sizeof(int));

	int ncoFreq; 
	int lutAddr;
	int deciSignalRe, deciSignalIm;
	int angDeciSignal, theta, piAdd,lpf;
	int cic_time, sampler_time,cpxMult_time;
	int auxRe,auxIm;
	/***************************************************************************
	int ncoInitFreq = ptr->InitFreq[rt_core_id()];
	int vgaMant = ptr->vgaMant[rt_core_id()];
	int vgaExp = ptr->vgaExp[rt_core_id()];

	demod_mem * p;
	p = (ptr->str_demod[rt_core_id()]);
	mem_cic * str;
	str = (ptr->str_cic[rt_core_id()]);
	mem_cic * str1;
	str1 = (ptr->str_cicSmp[rt_core_id()]);
	sampler_mem * str_smp;
	str_smp = (ptr->str_smp[rt_core_id()]);
	***************************************************************************/

	for(iSymb = 0; iSymb<nSymb;iSymb++){	

		p->symbCount++;
		/*
		*	Partitioning of the input signal per symbol (160 samples/symbol)
		*/
//		printf("Partitioning the input signal\n");
		for(i0=0;i0<smplPerSymb;i0++){
			inputBlockRe[i0] = (int)creal(inputSignal[i0+smplPerSymb*iSymb]);
			inputBlockIm[i0] = (int)cimag(inputSignal[i0+smplPerSymb*iSymb]);
		}

		/*
		*	NCO update and LUT
		*/
		ncoFreq = p->ncoDFreq+ncoInitFreq;
		
		incAndOvFlow(p->thetaNco, ncoFreq, smplPerSymb, freqW, ncoTheta);
		p->thetaNco = ncoTheta[smplPerSymb-1];
        
/*		
		for(i0=0;i0<smplPerSymb;i0++){			
			lutAddr = ncoTheta[i0] >> (int)cabs(thetaW-freqW);
			ncoSignalRe[i0] = creal(ncoLut[lutAddr]);
			ncoSignalIm[i0] = -1*cimag(ncoLut[lutAddr]);
		}		
*/
		/*
		*	Complex Multiplier
		* floor(float)
		*/		
		cpxMult_time = rt_time_get_us();
		for (i0=0;i0<smplPerSymb;i0++){
			//cplxMult[i0] = inputBlock[i0]*ncoSignal[i0];
			lutAddr = ncoTheta[i0] >> (int) (freqW-thetaW);
			//cplxMultRe[i0] = (inputBlockRe[i0]*ncoSignalRe[i0]-inputBlockIm[i0]*ncoSignalIm[i0])>>(ncoAmpW-1);
			auxRe = (inputBlockRe[i0]*((int)creal(ncoLut[lutAddr]))-inputBlockIm[i0]*-1*((int)cimag(ncoLut[lutAddr])))>>(ncoAmpW-1);
			auxIm = (inputBlockRe[i0]*-1*((int)cimag(ncoLut[lutAddr]))+inputBlockIm[i0]*((int)creal(ncoLut[lutAddr])))>>(ncoAmpW-1);
			inputBlockRe[i0] = auxRe;
			inputBlockIm[i0] = auxIm;
//			cplxMultIm[i0] = (inputBlockRe[i0]*ncoSignalIm[i0]+inputBlockIm[i0]*ncoSignalRe[i0])>>(ncoAmpW-1);
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
		cicFilter(inputBlockRe,inputBlockIm,str,mfSignalRe,mfSignalIm,deciRate,delayIdx,smplPerSymb);
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
		angDeciSignal = ((int)atan2(deciSignalIm,deciSignalRe)<<(cordicW-1))/PI;
//		angDeciSignal = atan2(deciSignalIm,deciSignalRe)*pow(2,cordicW-1)/PI;
		theta = angDeciSignal>>(cordicW-thetaW);
		/*
		*	PLL Loop Filter
		*/
		piAdd = kpUInt*theta+(p->lfAcc>>((int)cabs(kpExp-kiExp)));		
		p->lfAcc +=kiUInt*theta;
		lpf = piAdd>>((int)cabs(freqW-thetaW-kpExp));
		p->ncoDFreq = lpf;	

	}
rt_free(MEM_ALLOC,inputBlockRe,smplPerSymb*(sizeof(int)));
rt_free(MEM_ALLOC,inputBlockIm,smplPerSymb*(sizeof(int)));
//rt_free(MEM_ALLOC,cplxMultRe,smplPerSymb*(sizeof(int)));   
//rt_free(MEM_ALLOC,cplxMultIm,smplPerSymb*(sizeof(int)));   
//rt_free(MEM_ALLOC,ncoTheta,(smplPerSymb)*sizeof(int));
return 0;
}






