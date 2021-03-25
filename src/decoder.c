#include "decoder.h"

void clearDecoder(FreqsRecord_T * PTT_DP_LIST,PTTService_T * wpckg, mem_cic * str_cic, mem_cic * str_cicSmp, sampler_mem * str_smp, demod_mem * str_demod)
{
	int i0;
	str_smp->lutSign = 1;
	str_smp->symbLock = 0;
	str_smp->delayAcc = 0;
	str_smp->prevDelay = 0; 

	//CIC filter of demod  
	str_cic->accRe = 0;
	str_cic->accIm = 0;    
	str_cic->accDlyIdx = delayIdx-1;   
	for (i0 = 0; i0<delayIdx;i0++){
		str_cic->previousAccRe[i0]=0;
		str_cic->previousAccIm[i0]=0;
	}
  
	//CIC filter of sampler
	str_cicSmp->accRe = 0;
	str_cicSmp->accIm = 0;    
	str_cicSmp->accDlyIdx = delaySmp-1;    
	for (i0 = 0; i0<delaySmp;i0++){
		str_cicSmp->previousAccRe[i0]=0;
		str_cicSmp->previousAccIm[i0]=0;
	}
  
	for (i0 = 0; i0<nSymb;i0++){
		str_demod->symbLock[i0] = 0;
		str_demod->symbOut[i0] = 0;
	}
	str_demod->ncoDFreq = 0;
	str_demod->symbCount = 0;
	str_demod->lfAcc = 0;
	str_demod->thetaNco = 0;
  
	PTT_DP_LIST->freq_idx = 0;
	PTT_DP_LIST->freq_amp = 0;
	PTT_DP_LIST->detect_state = FREQ_NONE;
	PTT_DP_LIST->timeout = 0;
  
  
	wpckg->frameType = 0;
	wpckg->timeTag = 0;
	wpckg->errorCode = 0;
	wpckg->carrierAbs = 0;
	wpckg->carrierFreq = 0;
	wpckg->msgByteLength = 0;
	wpckg->status = PTT_FREE;
	for(i0=0;i0<nSymb;i0++){
		wpckg->symb_array[i0] = 0;
	}
	for(i0=0;i0<35;i0++){
		wpckg->userMsg[i0] = 0;
	}
	wpckg->synch_patternA = 0;
	wpckg->synch_patternB = 0;
	wpckg->total_symbol_cnt = 0;
	wpckg->bit_cnt = 0;
	wpckg->symb_cnt = 0;
}

void UpdateTimeout(FreqsRecord_T * PTT_DP_LIST[NoD], PTTService_T * wpckg[NoD])
{
	int dId;
	for (dId = 0; dId < NoD; dId++) {
		if (PTT_DP_LIST[dId]->timeout == 1) { // frequency timeout
			PTT_DP_LIST[dId]->timeout = 0;
			PTT_DP_LIST[dId]->freq_amp = 0;
			PTT_DP_LIST[dId]->freq_idx = 0;
			PTT_DP_LIST[dId]->detect_state = FREQ_NONE;
			wpckg[dId]->status=PTT_FREE;
//			printf("TO Clearing %d\n", dId);
		} else {
			PTT_DP_LIST[dId]->timeout--;
		}
	}
}
void bitDetection(FreqsRecord_T * PTT_DP_LIST[NoD], demodArg_t * ptr)
{
	int dId,iSymb,i2;
	dId = rt_core_id();
	for (dId=0;dId<NoD;dId++){
		if(PTT_DP_LIST[dId]->detect_state==FREQ_DECODING){
//			printf("Starting bit detection process channel %d\n",dId);
			for(iSymb = 0;iSymb<nSymb;iSymb++){
				if(ptr->str_demod[dId]->symbLock[iSymb]){
					ptr->wpckg[dId]->total_symbol_cnt++;
					if(ptr->wpckg[dId]->status==PTT_FRAME_SYNCH){
						frameSynch(ptr->wpckg[dId],ptr->str_demod[dId]->symbOut[iSymb]);   
					}else if(ptr->wpckg[dId]->status==PTT_DATA){
						readData(ptr->wpckg[dId],ptr->str_demod[dId]->symbOut[iSymb]);
						if(ptr->wpckg[dId]->status==PTT_READY){
//							fill the package and clear the decoder
							//printf("Clearing decoder %d\n",dId);
//							clearDecoder(PTT_DP_LIST[dId],wpckg[dId], ptr->str_cic[dId], ptr->str_cicSmp[dId], ptr->str_smp[dId], ptr->str_demod[dId]);
					ptr->activeList--;
                //DEBUG Purpose                
						}
					}else if(ptr->wpckg[dId]->status==PTT_ERROR){
						printf("Clearing decoder %d\n",dId);
			//			clearDecoder(PTT_DP_LIST[dId],ptr->wpckg[dId], ptr->str_cic[dId], ptr->str_cicSmp[dId], ptr->str_smp[dId], ptr->str_demod[dId]);
						ptr->activeList--;
					}
				}
			}
		}
	}//END FOR SCROLLING CHANNELS
}

void prllBitDetection(demodArg_t * ptr)
{
	int dId,iSymb,i2;
	dId = rt_core_id();
//	printf("Starting bit detection process channel %d\n",dId);
	for(iSymb = 0;iSymb<nSymb;iSymb++){
		if(ptr->str_demod[dId]->symbLock[iSymb]){
			ptr->wpckg[dId]->total_symbol_cnt++;
			if(ptr->wpckg[dId]->status==PTT_FRAME_SYNCH){
				frameSynch(ptr->wpckg[dId],ptr->str_demod[dId]->symbOut[iSymb]);   
			}else if(ptr->wpckg[dId]->status==PTT_DATA){
				readData(ptr->wpckg[dId],ptr->str_demod[dId]->symbOut[iSymb]);
				if(ptr->wpckg[dId]->status==PTT_READY){
//				fill the package and clear the decoder
				//printf("Clearing decoder %d\n",dId);
//					clearDecoder(PTT_DP_LIST[dId],wpckg[dId], ptr->str_cic[dId], ptr->str_cicSmp[dId], ptr->str_smp[dId], ptr->str_demod[dId]);
					ptr->activeList--;
//DEBUG Purpose                	
				}
			}else if(ptr->wpckg[dId]->status==PTT_ERROR){
				printf("Clearing decoder %d\n",dId);
//				clearDecoder(PTT_DP_LIST[dId],ptr->wpckg[dId], ptr->str_cic[dId], ptr->str_cicSmp[dId], ptr->str_smp[dId], ptr->str_demod[dId]);
				ptr->activeList--;
			}
		}
	}
}

void decoder(int complex * inputSignal, demodArg_t * ptr, FreqsRecord_T * PTT_DP_LIST[NoD], PTTPackage_T * outputPckg[NoD])
{

	int dId,i2,iSymb,activeDecoders;
	if(ptr->activeList>0){
		activeDecoders = ptr->activeList;
//		printf("activeList: %d\n",activeDecoders);
		prlpttA2Demod(inputSignal, ptr);
//		rt_team_fork(activeDecoders, (void *) prlpttA2Demod,(void *) ptr);
		rt_team_fork(activeDecoders, (void *) prllBitDetection,(void *) ptr);
//		bitDetection(PTT_DP_LIST, ptr);
	}
	// Pass the package service to output package
	for(dId=0;dId<NoD;++dId){
		if(ptr->wpckg[dId]->status==PTT_READY){
			outputPckg[dId]->status = ptr->wpckg[dId]->status;
			outputPckg[dId]->carrierFreq = ptr->wpckg[dId]->carrierFreq;
			outputPckg[dId]->carrierAbs = ptr->wpckg[dId]->carrierAbs;
			outputPckg[dId]->msgByteLength = ptr->wpckg[dId]->msgByteLength;
			memcpy(outputPckg[dId]->userMsg,ptr->wpckg[dId]->userMsg, outputPckg[dId]->msgByteLength*sizeof(int *));
		}
	}
}
