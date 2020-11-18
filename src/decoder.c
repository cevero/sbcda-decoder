#include "decoder.h"

void clearDecoder(FreqsRecord_Typedef * PTT_DP_LIST,PTTPackage_Typedef * wpckg, mem_cic * str_cic, mem_cic * str_cicSmp, sampler_mem * str_smp, demod_mem * str_demod)
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

void UpdateTimeout(FreqsRecord_Typedef * PTT_DP_LIST[NUMBER_OF_DECODERS], PTTPackage_Typedef * wpckg[NUMBER_OF_DECODERS])
{
	int iCh;
	for (iCh = 0; iCh < NUMBER_OF_DECODERS; iCh++) {
		if (PTT_DP_LIST[iCh]->timeout == 1) { // frequency timeout
			PTT_DP_LIST[iCh]->timeout = 0;
			PTT_DP_LIST[iCh]->freq_amp = 0;
			PTT_DP_LIST[iCh]->freq_idx = 0;
			PTT_DP_LIST[iCh]->detect_state = FREQ_NONE;
			wpckg[iCh]->status=PTT_FREE;
			printf("TO Clearing %d\n", iCh);
		} else {
			PTT_DP_LIST[iCh]->timeout--;
		}
	}
}

void decoder(int complex *inputSignal, FreqsRecord_Typedef * PTT_DP_LIST[NUMBER_OF_DECODERS], PTTPackage_Typedef * wpckg[NUMBER_OF_DECODERS],demod_mem * str_demod[NUMBER_OF_DECODERS], mem_cic * str_cic[NUMBER_OF_DECODERS], mem_cic * str_cicSmp[NUMBER_OF_DECODERS], sampler_mem * str_smp[NUMBER_OF_DECODERS]){
	
	int f,tmp0,n,i1;
	int iCh,vga, activeList;
	int vgaExp[NUMBER_OF_DECODERS],vgaMant[NUMBER_OF_DECODERS];
	int InitFreq[NUMBER_OF_DECODERS];

//	tmp0 = detectLoop(inputSignal, PTT_DP_LIST);	
	
	//DEBUG Purpose
	if(tmp0){
		printf("New PTT(s) detected!\nStatus of all decoders:\n");
		for(n=0;n<NUMBER_OF_DECODERS;n++){
			printf("ch:%d state: %d freq: %d\n",n,PTT_DP_LIST[n]->detect_state, PTT_DP_LIST[n]->freq_idx);
		}
		tmp0=0;
	}
  //Setup Parameters: Frequency, Gain, Controls status of pckg and Detect.
	for (iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
		if(PTT_DP_LIST[iCh]->detect_state==FREQ_DETECTED_TWICE){
			int vga = VgaGain(PTT_DP_LIST[iCh]->freq_amp);
			vgaExp[iCh] = -1*(vga&0x3F);
			vgaMant[iCh] = (vga>>6)&0xFF;
			InitFreq[iCh] = PTT_DP_LIST[iCh]->freq_idx<<9;
			printf("mant %d exp %d\n", vgaMant[iCh],vgaExp[iCh]);
			PTT_DP_LIST[iCh]->detect_state=FREQ_DECODING;
			wpckg[iCh]->status=PTT_FRAME_SYNCH;
		}
	}

	//decodes signals from active channels
  for (iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
    if(PTT_DP_LIST[iCh]->detect_state==FREQ_DECODING){
      pttA2Demod(inputSignal, InitFreq[iCh], vgaMant[iCh],          vgaExp[iCh], str_demod[iCh], str_cic[iCh], str_cicSmp[iCh], str_smp[iCh]);

      for(i1 = 0;i1<nSymb;i1++){
				if(str_demod[iCh]->symbLock[i1]){
					wpckg[iCh]->total_symbol_cnt++;
					if(wpckg[iCh]->status==PTT_FRAME_SYNCH){
						frameSynch(wpckg[iCh],str_demod[iCh]->symbOut[i1]);		
					}else if(wpckg[iCh]->status==PTT_DATA){
						readData(wpckg[iCh],str_demod[iCh]->symbOut[i1]);
						if(wpckg[iCh]->status==PTT_READY){
							//fill the package and clear the decoder
							printf("ready!\n");
							printf("|%d|\n",iCh);
							for(int i2=0;i2<wpckg[iCh]->msgByteLength;i2++){
								printf("%x\n",wpckg[iCh]->userMsg[i2]);
							}
							printf("Clearing decoder %d\n",iCh);
							clearDecoder(PTT_DP_LIST[iCh],wpckg[iCh], str_cic[iCh], str_cicSmp[iCh], str_smp[iCh], str_demod[iCh]);
						}
					}else if(wpckg[iCh]->status==PTT_ERROR){
						printf("Clearing decoder %d\n",iCh);
						clearDecoder(PTT_DP_LIST[iCh],wpckg[iCh], str_cic[iCh], str_cicSmp[iCh], str_smp[iCh], str_demod[iCh]);
					}
				}
			}	
    }
  }
}
