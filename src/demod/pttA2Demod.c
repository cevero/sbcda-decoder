#include "pttA2Demod.h"
#include "ncoLut.h"
#include <omp.h>


#define NUMTHREAD 8

void pttA2Demod(int complex * inputSignal,int ncoInitFreq,int vgaMant,int
        vgaExp, demod_mem * p, mem_cic * str, mem_cic * str1, sampler_mem * str_smp)
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

	for(iSymb = 0; iSymb<nSymb;iSymb++)
    {
#pragma omp atomic
		p->symbCount++;

		/*
		*	Partitioning of the input signal per symbol (160 samples/symbol)
		*/
#pragma omp critical
        {
		for(i0=0;i0<smplPerSymb;i0++)
        {
			inputBlock[i0] = inputSignal[i0+smplPerSymb*iSymb];
		}

		/*
		*	NCO update and LUT
		*/
		ncoFreq = p->ncoDFreq+ncoInitFreq;
		
		incAndOvFlow(p->thetaNco, ncoFreq, smplPerSymb, freqW, ncoTheta);
		p->thetaNco = ncoTheta[smplPerSymb-1];
		for(i0=0;i0<smplPerSymb;i0++)
        {			
			lutAddr = ncoTheta[i0] >> abs(thetaW-freqW);
			ncoSignal[i0] = creal(ncoLut[lutAddr])-cimag(ncoLut[lutAddr])*I;
		}		

		/*
		*	Complex Multiplier
		* floor(float)
		*/		

        int SMPperSymb = smplPerSymb;
		for (i0=0;i0<SMPperSymb;i0++)
        {
			cplxMult[i0] = inputBlock[i0]*ncoSignal[i0];
			cplxMult[i0] = ((float complex) cplxMult[i0])*pow(2,-(ncoAmpW-1));
			cplxMult[i0] = floor(creal(cplxMult[i0]))+floor(cimag(cplxMult[i0]))*I;
		}
        }//critend
		/*
		*	Matched filter and decimation
		*/
		cicFilterCplxStep(cplxMult,str,mfSignal,deciRate,delayIdx,smplPerSymb);

		/*
		*	VGA input mfSignal output demodSignal
		*/
        
		for(i0=0;i0<smplPerSymb/deciRate;i0++)
        {
			vgaSignal[i0] = floor((creal(mfSignal[i0])*vgaMant)*pow(2,vgaExp-vgaMantW))+floor((cimag(mfSignal[i0])*vgaMant)*pow(2,vgaExp-vgaMantW))*I;
			demodSignal[i0] = cimag(vgaSignal[i0]);
		}

       // }
		/*
		*	Timer recovering SAMPLER
		*/
		
		sampler(demodSignal, str_smp, str1);
		p->symbOut[iSymb] = str_smp->symbOut;
		p->symbLock[iSymb] = str_smp->symbLock;

		/*	CIC2 */

		deciSignal = 0+0*I;
		for (i0 = 0;i0<deciRateSmp;i0++)
        {
			deciSignal = deciSignal+vgaSignal[i0];
		}
		int absDeciSignal = cabsf(deciSignal);
		int angDeciSignal = atan2(cimag(deciSignal),creal(deciSignal))*pow(2,cordicW-1)/PI;
		int theta = angDeciSignal*pow(2,(thetaW-cordicW));

		/*	PLL Loop Filter*/

		int piAdd;
        piAdd = kpUInt*theta+(p->lfAcc>>(abs(kpExp-kiExp)));		
		p->lfAcc +=kiUInt*theta;
		int lpf; 
		lpf = piAdd>>(abs(freqW-thetaW-kpExp));
		p->ncoDFreq = lpf;
	}
}
