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
//			printf("TO Clearing %d\n", iCh);
		} else {
			PTT_DP_LIST[iCh]->timeout--;
		}
	}
}

void decoder(int complex *inputSignal, FreqsRecord_Typedef * PTT_DP_LIST[NUMBER_OF_DECODERS], 
	PTTPackage_Typedef * wpckg[NUMBER_OF_DECODERS], int InitFreq[NUMBER_OF_DECODERS], 
	int vgaMant[NUMBER_OF_DECODERS], int vgaExp[NUMBER_OF_DECODERS], 
	demod_mem * str_demod[NUMBER_OF_DECODERS], 
	mem_cic * str_cic[NUMBER_OF_DECODERS], 
	mem_cic * str_cicSmp[NUMBER_OF_DECODERS], 
	sampler_mem * str_smp[NUMBER_OF_DECODERS])
{
	
	int iCh,i2,iSymb;
	
	for (iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
		if(PTT_DP_LIST[iCh]->detect_state==FREQ_DECODING){
//      printf("Starting demod process channel %d\n",iCh);
//      demod_time = rt_time_get_us();
//			decod_per_channel = rt_time_get_us();
			pttA2Demod(inputSignal, InitFreq[iCh], vgaMant[iCh], vgaExp[iCh], str_demod[iCh], str_cic[iCh], str_cicSmp[iCh], str_smp[iCh]);

  //      demod_time = rt_time_get_us()-demod_time;
//	printf("Demod time: %d ms \n",demod_time/1000);
			for(iSymb = 0;iSymb<nSymb;iSymb++){
				if(str_demod[iCh]->symbLock[iSymb]){
//		    printf("smb %d",str_demod[iCh]->symbOut[i1]);
					wpckg[iCh]->total_symbol_cnt++;
					if(wpckg[iCh]->status==PTT_FRAME_SYNCH){
						frameSynch(wpckg[iCh],str_demod[iCh]->symbOut[iSymb]);   
					}else if(wpckg[iCh]->status==PTT_DATA){
						readData(wpckg[iCh],str_demod[iCh]->symbOut[iSymb]);
						if(wpckg[iCh]->status==PTT_READY){
						
                //fill the package and clear the decoder
							printf("ready!\n");
							printf("|%d|\n",iCh);
							printf("Freq: %d Abs: %d Length: %d\n",wpckg[iCh]->carrierFreq,
							 wpckg[iCh]->carrierAbs, wpckg[iCh]->msgByteLength);
							for(i2=0;i2<wpckg[iCh]->msgByteLength;i2++){
								printf("%x\n",wpckg[iCh]->userMsg[i2]);
							}
							//printf("Clearing decoder %d\n",iCh);

							clearDecoder(PTT_DP_LIST[iCh],wpckg[iCh], str_cic[iCh], str_cicSmp[iCh], str_smp[iCh], str_demod[iCh]);
                //DEBUG Purpose                
						}
					}else if(wpckg[iCh]->status==PTT_ERROR){
						printf("Clearing decoder %d\n",iCh);
						clearDecoder(PTT_DP_LIST[iCh],wpckg[iCh], str_cic[iCh], str_cicSmp[iCh], str_smp[iCh], str_demod[iCh]);
					}
				}
			}
//			decod_per_channel= rt_time_get_us()-decod_per_channel;
//	printf("decod_time %d: %d us\n",iCh,decod_per_channel/1000);
		}
//	rt_gpio_set_pin_value(0,TRIGGER,0);
    }//END FOR SCROLLING CHANNELS
  // rt_gpio_set_pin_value(0,TRIGGER,0);
}
