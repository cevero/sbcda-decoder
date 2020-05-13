#include "pttA2Demod.h"
#include "ncoLut.h"

void pttA2Demod(int complex * inputSignal,int ncoInitFreq,int vgaMant,int vgaExp, demod_mem * p, mem_cic * str, mem_cic * str1, sampler_mem * str_smp)
{
	int iSymb, i0;
	
	int complex inputBlock[smplPerSymb];
	int complex ncoSignal[smplPerSymb];
	float complex cplxMult[smplPerSymb];
	float complex mfSignal[] = {[0 ... smplPerSymb/deciRate-1] = 0+0*I};
	int ncoFreq, demodSignal[smplPerSymb/deciRate];	
	int ncoTheta[smplPerSymb];
	int complex vgaSignal[smplPerSymb/deciRate];
	int lutAddr;
	int complex deciSignal;

	for(iSymb = 0; iSymb<nSymb;iSymb++){
		p->symbCount++;

		/*
		*	Partitioning of the input signal per symbol (160 samples/symbol)
		*/
		for(i0=0;i0<smplPerSymb;i0++){
			inputBlock[i0] = inputSignal[i0+smplPerSymb*iSymb];
			//printf("[%d] inputBlock %f %f i\n",i0+1, creal(inputBlock[i0]),cimag(inputBlock[i0]));
		}

		/*
		*	NCO update and LUT
		*/
		ncoFreq = p->ncoDFreq+ncoInitFreq;
		//printf("InitFreq %d p->ncoDfreq %d ncoFreq %d \n", ncoInitFreq, p->ncoDFreq, ncoFreq);
		
		incAndOvFlow(p->thetaNco, ncoFreq, smplPerSymb, freqW, ncoTheta);
		p->thetaNco = ncoTheta[smplPerSymb-1];
		//printf("p->ncoTheta %d ncoTheta[] %d \n", p->thetaNco, ncoTheta[smplPerSymb-1]);
		
		for(i0=0;i0<smplPerSymb;i0++){			
			lutAddr = ncoTheta[i0] >> abs(thetaW-freqW);
			//printf("lutAddr: %d ncoLut %f %f i\n", lutAddr, creal(ncoLut[lutAddr]),cimag(ncoLut[lutAddr]));
			ncoSignal[i0] = creal(ncoLut[lutAddr])-cimag(ncoLut[lutAddr])*I;
			//printf("[%d] ncoSignal %f %f i\n",i0, creal(ncoSignal[i0]),cimag(ncoSignal[i0]));
		}		

		/*
		*	Complex Multiplier
		* floor(float)
		*/		
		for (i0=0;i0<smplPerSymb;i0++){
			cplxMult[i0] = inputBlock[i0]*ncoSignal[i0];
			cplxMult[i0] = ((float complex) cplxMult[i0])*pow(2,-(ncoAmpW-1));
			cplxMult[i0] = floor(creal(cplxMult[i0]))+floor(cimag(cplxMult[i0]))*I;
			//printf("[%d] cplxMult %f %f i\n",i0, creal(cplxMult[i0]),cimag(cplxMult[i0]));
		}

		/*
		*	Matched filter and decimation
		*/
		cicFilterCplxStep(cplxMult,str,mfSignal,deciRate,delayIdx,smplPerSymb);

		// for (i0=0;i0<nSymb;i0++){
		// 	printf("[%d]cic %f %f i\n",i0,creal(mfSignal[i0]),cimag(mfSignal[i0]));
		// }
		/*
		*	VGA input mfSignal output demodSignal
		*/
		//printf("MANT %d EXP %d\n",vgaMant,vgaExp);
		for(i0=0;i0<smplPerSymb/deciRate;i0++){
			vgaSignal[i0] = floor((creal(mfSignal[i0])*vgaMant)*pow(2,vgaExp-vgaMantW))+floor((cimag(mfSignal[i0])*vgaMant)*pow(2,vgaExp-vgaMantW))*I;
			//printf("[%d]vga %f %f i\n",i0,creal(vgaSignal[i0]),cimag(vgaSignal[i0]));
			demodSignal[i0] = cimag(vgaSignal[i0]);
			//printf("demodSignal %d\n",demodSignal[i0]);
		}
		/*
		*	Timer recovering SAMPLER
		*/
		
		sampler(demodSignal, str_smp, str1);
		p->symbOut[iSymb] = str_smp->symbOut;
		p->symbLock[iSymb] = str_smp->symbLock;
		//printf("[%d] %d\n",str_smp->symbLock,str_smp->symbOut);

		/*
		*	CIC2
		*/
		deciSignal = 0+0*I;
		for (i0 = 0;i0<deciRateSmp;i0++){
			deciSignal = deciSignal+vgaSignal[i0];
		}
		//printf("deciSignal %f %f i\n",creal(deciSignal),cimag(deciSignal));
		int absDeciSignal = cabsf(deciSignal);
		int angDeciSignal = atan2(cimag(deciSignal),creal(deciSignal))*pow(2,cordicW-1)/PI;
		//printf("|%d| /_ %d\n",absDeciSignal,angDeciSignal);
		int theta = angDeciSignal*pow(2,(thetaW-cordicW));
		//printf("theta %d\n",theta);
		/*
		*	PLL Loop Filter
		*/

		int piAdd = kpUInt*theta+(p->lfAcc>>(abs(kpExp-kiExp)));		
		p->lfAcc +=kiUInt*theta;
		int lpf = piAdd>>(abs(freqW-thetaW-kpExp));
		p->ncoDFreq = lpf;
		//printf("piAdd %d | p->lfAcc %d | lpf %d \n",piAdd, p->lfAcc,lpf);
	}
}