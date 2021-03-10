#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <complex.h>
#include "demod/pttA2Demod.h"
#include "service.h"
#include "detect_loop.h"

void clearDecoder(FreqsRecord_Typedef * PTT_DP_LIST,PTTPackage_Typedef * wpckg, mem_cic * str_cic, 
	mem_cic * str_cicSmp, sampler_mem * str_smp, demod_mem * str_demod);

void UpdateTimeout(FreqsRecord_Typedef * PTT_DP_LIST[NUMBER_OF_DECODERS], 
	PTTPackage_Typedef * wpckg[NUMBER_OF_DECODERS]);

//void decoder(int complex *inputSignal, FreqsRecord_Typedef * PTT_DP_LIST[NUMBER_OF_DECODERS], PTTPackage_Typedef * wpckg[NUMBER_OF_DECODERS], int * InitFreq, int * vgaMant, int * vgaExp, demod_mem * str_demod[NUMBER_OF_DECODERS], mem_cic * str_cic[NUMBER_OF_DECODERS], mem_cic * str_cicSmp[NUMBER_OF_DECODERS], sampler_mem * str_smp[NUMBER_OF_DECODERS]);
void decoder(int complex * inputSignal, demodArg_t * ptr, FreqsRecord_Typedef * PTT_DP_LIST[NUMBER_OF_DECODERS], PTTPackage_Typedef * wpckg[NUMBER_OF_DECODERS]);
