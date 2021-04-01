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

	int ncoInitFreq;
	int vgaMant;
	int vgaExp;

//	***************************************************************************
	int dId = rt_core_id()+(NoC*ptr->nSeq);
	int * inputBlockRe;
	inputBlockRe = ptr->inputBlockRe[rt_core_id()];
	int * inputBlockIm;
	inputBlockIm = ptr->inputBlockIm[rt_core_id()];
	int * ncoTheta;
	demod_mem * p;
	mem_cic * str;
	mem_cic * str1;
	sampler_mem * str_smp;

	ncoTheta = ptr->ncoTheta[rt_core_id()];
	ncoInitFreq = ptr->InitFreq[dId];
	vgaMant = ptr->vgaMant[dId];
	vgaExp = ptr->vgaExp[dId];
	p = (ptr->str_demod[dId]);
	str = (ptr->str_cic[dId]);
	str1 = (ptr->str_cicSmp[dId]);
	str_smp = (ptr->str_smp[dId]);
//	***************************************************************************
/*	if(dId!=1)
		printf("(%d) %d %d\n",dId,inputBlockRe[0], inputBlockIm[0]);
	return 0;
*/	iSymb = ptr->iSymb;
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
	
/*	if(dId!=1)
		printf("(%d) %d %d\n",dId,inputBlockRe[0], inputBlockIm[0]);
	return 0;
*/
	if(iSymb==0){
//		printf("cpxMult time: %d us\n",cpxMult_time);
	}
	/*
	*	Matched filter and decimation
	*/

	cicFilter(inputBlockRe,inputBlockIm,str,mfSignalRe,mfSignalIm,deciRate,delayIdx,smplPerSymb);
	
/*	if(dId!=1)
		printf("(%d) %d %d\n",dId,mfSignalRe[0], mfSignalIm[0]);
	return 0;*/

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

/*	if(dId!=1){
		printf("(%d) %d\n",dId,str_smp->symbOut);
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

//	rt_team_barrier();
	return 0;
}

int prlpttA2Demod(int complex * inputSignal, demodArg_t * arg)
{
	int i0, iSymb;
	int activeList = (arg->activeList>NoC)? NoC-(arg->nSeq*(NoD-arg->activeList)):arg->activeList;
	
	for(i0=0;i0<activeList;++i0){
		arg->inputBlockRe[i0] = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
		arg->inputBlockIm[i0] = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
		arg->ncoTheta[i0] = rt_alloc(MEM_ALLOC,smplPerSymb*sizeof(int));
	}

	for(iSymb = 0; iSymb<nSymb;iSymb++){

		/*
		*	Partitioning of the input signal per symbol (160 samples/symbol)
		*/
		for(i0=0;i0<smplPerSymb;i0++){
			arg->inputBlockRe[0][i0] = creal(inputSignal[i0+smplPerSymb*iSymb]);
			arg->inputBlockIm[0][i0] = cimag(inputSignal[i0+smplPerSymb*iSymb]);
		}

		for(i0=1;i0<activeList;++i0){
			memcpy(arg->inputBlockRe[i0],arg->inputBlockRe[0],smplPerSymb*sizeof(int));
			memcpy(arg->inputBlockIm[i0],arg->inputBlockIm[0],smplPerSymb*sizeof(int));
		}
		arg->iSymb = iSymb;
		rt_team_fork(activeList, (void *) pttA2DemodStep, (void *) arg);
	}
	for(i0=0;i0<activeList;++i0){
		rt_free(MEM_ALLOC,arg->inputBlockRe[i0],smplPerSymb*sizeof(int));
		rt_free(MEM_ALLOC,arg->inputBlockIm[i0],smplPerSymb*sizeof(int));
		rt_free(MEM_ALLOC,arg->ncoTheta[i0],smplPerSymb*sizeof(int));
	}
	return 0;
}

