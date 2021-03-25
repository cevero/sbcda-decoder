#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <complex.h>
#include "demod/pttA2Demod.h"
#include "service.h"
#include "detect_loop.h"

void clearDecoder(FreqsRecord_T * PTT_DP_LIST,PTTService_T * wpckg, mem_cic * str_cic, 
	mem_cic * str_cicSmp, sampler_mem * str_smp, demod_mem * str_demod);

void UpdateTimeout(FreqsRecord_T * PTT_DP_LIST[NoD], PTTService_T * wpckg[NoD]);

//void decoder(int complex *inputSignal, FreqsRecord_Typedef * PTT_DP_LIST[NoD], PTTPackage_Typedef * wpckg[NoD], int * InitFreq, int * vgaMant, int * vgaExp, demod_mem * str_demod[NoD], mem_cic * str_cic[NoD], mem_cic * str_cicSmp[NoD], sampler_mem * str_smp[NoD]);
void decoder(int complex * inputSignal, demodArg_t * ptr, FreqsRecord_T * PTT_DP_LIST[NoD], PTTPackage_T * outputPckg[NoD]);
