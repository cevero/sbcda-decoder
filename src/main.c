#include "Gap8.h" 
#include "rt/rt_api.h"
#include "decoder.h"
#include "inputSignalMin.h"
#include "../lib/kiss_fft.h"
#include "../lib/_kiss_fft_guts.h"
#include "service.h"
#define NUMBER_OF_SAMPLES 47360 
int main()
{
  rt_freq_set(RT_FREQ_DOMAIN_FC,200000000);
  int n0;
  printf("--------> Loading Input Signal %d <-------\n\n",rt_freq_get(RT_FREQ_DOMAIN_FC));
 //int complex input [1280*36];
 printf("--------> Finish Load Input Signal <-------\n\n");   
   
  //sampler memory
  int i;
 
  sampler_mem * str_smp[NUMBER_OF_DECODERS];
  for (i = 0; i < NUMBER_OF_DECODERS; ++i){
    str_smp[i] = rt_alloc(MEM_ALLOC,sizeof(sampler_mem));
    str_smp[i]->smplBuffer = rt_alloc(MEM_ALLOC,2*smplPerSymb*(sizeof(int)));
  }

  //CIC filter of demod
  mem_cic * str_cic[NUMBER_OF_DECODERS];
  for (i = 0; i < NUMBER_OF_DECODERS; ++i){
    str_cic[i] = rt_alloc(RT_ALLOC_FC_DATA,sizeof(mem_cic));
    str_cic[i]->previousAccRe = rt_alloc(RT_ALLOC_FC_DATA,delayIdx*sizeof(int));
    str_cic[i]->previousAccIm = rt_alloc(RT_ALLOC_FC_DATA,delayIdx*sizeof(int));
  }
  
  //CIC filter of sampler
  mem_cic * str_cicSmp[NUMBER_OF_DECODERS];
  for (i = 0; i < NUMBER_OF_DECODERS; ++i){
    str_cicSmp[i]= rt_alloc(MEM_ALLOC,sizeof(mem_cic));
    str_cicSmp[i]->previousAccRe = rt_alloc(MEM_ALLOC,delaySmp*sizeof(int));
    str_cicSmp[i]->previousAccIm = rt_alloc(MEM_ALLOC,delaySmp*sizeof(int));
  }

  //demod_mem stores accumulators, symbOut and symbLock
  demod_mem * str_demod[NUMBER_OF_DECODERS];
  for(i=0;i<NUMBER_OF_DECODERS;i++){
    str_demod[i] = rt_alloc(MEM_ALLOC,sizeof(demod_mem));
    str_demod[i]->symbLock = rt_alloc(MEM_ALLOC,nSymb*sizeof(int));
    str_demod[i]->symbOut = rt_alloc(MEM_ALLOC,nSymb*sizeof(int));
  }

  // struct stores the interface (detect to demod) parameters
  FreqsRecord_Typedef *PTT_DP_LIST[NUMBER_OF_DECODERS];
  for(i=0;i<NUMBER_OF_DECODERS;i++){
    PTT_DP_LIST[i] = rt_alloc(MEM_ALLOC,sizeof(FreqsRecord_Typedef));
  }
  

  // This struct is used to control the FSM and show bit results
  PTTPackage_Typedef * wpckg[NUMBER_OF_DECODERS];
  for(i=0;i<NUMBER_OF_DECODERS;i++){
    wpckg[i] = rt_alloc(MEM_ALLOC,sizeof(PTTPackage_Typedef));
  }

  int * prevIdx = rt_alloc(MEM_ALLOC,DFT_LENGTH*sizeof(int));
  for(i=0;i<DFT_LENGTH;i++){
	  prevIdx[i]=0;
  }

  printf("***------------ ALL READY --------------***\n");
  int complex *inputSignal = rt_alloc(MEM_ALLOC,WINDOW_LENGTH*sizeof(int complex));
  int tmp0=0,f=0,iCh,vga, activeList, nWind, spWind,iSymb,i2;
  int * vgaExp = rt_alloc(MEM_ALLOC,NUMBER_OF_DECODERS*sizeof(int));
  int * vgaMant = rt_alloc(MEM_ALLOC,NUMBER_OF_DECODERS*sizeof(int));
  int * InitFreq = rt_alloc(MEM_ALLOC,NUMBER_OF_DECODERS*sizeof(int));
  int detect_time=0, demod_time=0, decod_time=0, total_time=0,aux_time, decod_per_channel=0;
#ifdef DEBUG_DEMOD
  int debug = 0;
#endif

/*
//Calculate LUT for fft computation---------------------------*
cpx * we = rt_alloc(MEM_ALLOC,1024*sizeof(cpx));//-*
for(i=0;i<DFT_LENGTH/2;i++){//				     -*
	we[i].r=(cos(2*PI*i/DFT_LENGTH));//    		     -*
	we[i].i=-1*(sin(2*PI*i/DFT_LENGTH));//               -*
}//-----------------------------------------------------------*
*/

//  kiss_fft_cfg cfg;
//  cfg = kiss_fft_alloc(DFT_LENGTH,0,NULL,NULL);

  for(iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
    printf("Clearing decoder %d\n",iCh);
    clearDecoder(PTT_DP_LIST[iCh],wpckg[iCh], str_cic[iCh], str_cicSmp[iCh], str_smp[iCh], str_demod[iCh]);
  }

  #define NSIM (1)
  
  for(n0=0; n0<NSIM;n0++){
#ifdef DEBUG_DEMOD
	if(n0==1){
		debug = 1;
	}
#endif
for (nWind=0;nWind<NUMBER_OF_SAMPLES/WINDOW_LENGTH;nWind++){
	aux_time = rt_time_get_us();
//  for (nWind=0;nWind<2;nWind++){ //DEBUG DETECT PROCESS
    printf("***--------- Window processing ---------*** [%d]\n", nWind);
    
    // Performs input partitioning on the windows of 1280 samples
    memcpy(inputSignal,inputSignalMin+WINDOW_LENGTH*nWind,1280*sizeof(int complex));
    
    /* DEBUG SIGNAL WINDOW
    if(nWind>30){
	    printf("0 %d %d\n",inputSignal[0]);
	    printf("L %d %d\n",inputSignal[WINDOW_LENGTH-1]);
    }*/

   printf("Updating Timeout !\n");

   UpdateTimeout(PTT_DP_LIST,wpckg);
  
   detect_time = rt_time_get_us();
   //printf("Starting detection loop ! %d ms\n", detect_time/1000);

   tmp0 =  detectLoop(inputSignal, prevIdx, PTT_DP_LIST);
   
   detect_time = rt_time_get_us()-detect_time;
   printf("Detect: %d ms\n",detect_time/1000);
   //DEBUG Purpose
   if(tmp0){
     printf("New PTT(s) detected!\nStatus of all decoders:\n");
     for(iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
       printf("ch:%d state: %d freq: %4d amp: %4d\n",iCh,PTT_DP_LIST[iCh]->detect_state, PTT_DP_LIST[iCh]->freq_idx, PTT_DP_LIST[iCh]->freq_amp);
     }
     tmp0=0;
   } 

    //Setup Parameters: Frequency, Gain, Controls status of pckg and Detect.
    for (iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
      if(PTT_DP_LIST[iCh]->detect_state==FREQ_DETECTED_TWICE){
        vga = VgaGain(PTT_DP_LIST[iCh]->freq_amp);
        vgaExp[iCh] = -1*(vga&0x3F);
        vgaMant[iCh] = (vga>>6)&0xFF;
        InitFreq[iCh] = PTT_DP_LIST[iCh]->freq_idx<<9;
//        printf("[%d]: mant %d exp %d freq %d\n",iCh, vgaMant[iCh],vgaExp[iCh],PTT_DP_LIST[iCh]->freq_idx);
        PTT_DP_LIST[iCh]->detect_state=FREQ_DECODING;
        wpckg[iCh]->status=PTT_FRAME_SYNCH;
      }
    }

#ifdef DEBUG_DEMOD
   // DEBUG DEMOD AND DECOD PROCESSES  
    if(nWind==1 && debug==0){
	PTT_DP_LIST[0]->detect_state = FREQ_DECODING;
	PTT_DP_LIST[0]->freq_amp = 793;
	PTT_DP_LIST[0]->freq_idx = 1788;
	PTT_DP_LIST[0]->timeout = 100;
	vga = VgaGain(PTT_DP_LIST[0]->freq_amp);
	vgaExp[0] = -1*(vga&0x3F);
	vgaMant[0] = (vga>>6)&0xFF;
	InitFreq[0] = PTT_DP_LIST[0]->freq_idx<<9;
        printf("[%d]: mant %d exp %d\n",0, vgaMant[0],vgaExp[0]);
        wpckg[0]->status=PTT_FRAME_SYNCH;
	PTT_DP_LIST[1]->detect_state = FREQ_DECODING;
	PTT_DP_LIST[1]->freq_amp = 787;
	PTT_DP_LIST[1]->freq_idx = 397;
	PTT_DP_LIST[1]->timeout = 100;
	vga = VgaGain(PTT_DP_LIST[1]->freq_amp);
	vgaExp[1] = -1*(vga&0x3F);
	vgaMant[1] = (vga>>6)&0xFF;
	InitFreq[1] = PTT_DP_LIST[1]->freq_idx<<9;
        printf("[%d]: mant %d exp %d\n",0, vgaMant[1],vgaExp[1]);
        wpckg[1]->status=PTT_FRAME_SYNCH;
    }
#endif

    
    //decodes signals from active channels
    for (iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
      if(PTT_DP_LIST[iCh]->detect_state==FREQ_DECODING){
//      printf("Starting demod process channel %d\n",iCh);
        demod_time = rt_time_get_us();
//	decod_per_channel = rt_time_get_us();
        pttA2Demod(inputSignal, InitFreq[iCh], vgaMant[iCh], vgaExp[iCh], str_demod[iCh], str_cic[iCh], str_cicSmp[iCh], str_smp[iCh]);

        demod_time = rt_time_get_us()-demod_time;
	printf("Demod time: %d ms \n",demod_time/1000);
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
                for(i2=0;i2<wpckg[iCh]->msgByteLength;i2++){
                  printf("%x\n",wpckg[iCh]->userMsg[i2]);
                }
                printf("Clearing decoder %d\n",iCh);
                clearDecoder(PTT_DP_LIST[iCh],wpckg[iCh], str_cic[iCh], str_cicSmp[iCh], str_smp[iCh], str_demod[iCh]);
                //DEBUG Purpose                
              }
            }else if(wpckg[iCh]->status==PTT_ERROR){
              printf("Clearing decoder %d\n",iCh);
              clearDecoder(PTT_DP_LIST[iCh],wpckg[iCh], str_cic[iCh], str_cicSmp[iCh], str_smp[iCh], str_demod[iCh]);
            }
          }
        }
//     	decod_per_channel= rt_time_get_us()-decod_per_channel;
//	printf("decod_time %d: %d ms\n",iCh,decod_per_channel/1000);
      }
    }//END FOR SCROLLING CHANNELS
    
  }//END FOR SCROLLING WINDOWS
  total_time = rt_time_get_us()-aux_time;
  printf("total time %d\n",total_time/1000);
  }//END FOR NSIM 

  rt_free(MEM_ALLOC,prevIdx,DFT_LENGTH*sizeof(int));
  rt_free(MEM_ALLOC,inputSignal,WINDOW_LENGTH*sizeof(int complex));
  rt_free(MEM_ALLOC,vgaExp,NUMBER_OF_DECODERS*sizeof(int));
  rt_free(MEM_ALLOC,vgaMant,NUMBER_OF_DECODERS*sizeof(int));
  rt_free(MEM_ALLOC,InitFreq,NUMBER_OF_DECODERS*sizeof(int));
  
  for(i=0;i<NUMBER_OF_DECODERS;i++){
    rt_free(MEM_ALLOC,PTT_DP_LIST[i],sizeof(FreqsRecord_Typedef));
    rt_free(MEM_ALLOC,wpckg[i],sizeof(PTTPackage_Typedef));
    rt_free(MEM_ALLOC,str_smp[i]->smplBuffer,2*smplPerSymb*(sizeof(int)));   
    rt_free(MEM_ALLOC,str_smp[i],sizeof(sampler_mem));       
    rt_free(MEM_ALLOC,str_cicSmp[i]->previousAccRe,delaySmp*sizeof(int));
    rt_free(MEM_ALLOC,str_cicSmp[i]->previousAccIm,delaySmp*sizeof(int));
    rt_free(MEM_ALLOC,str_cicSmp[i],sizeof(mem_cic));
    rt_free(MEM_ALLOC,str_cic[i]->previousAccRe,delayIdx*sizeof(int));
    rt_free(MEM_ALLOC,str_cic[i]->previousAccIm,delayIdx*sizeof(int));
    rt_free(MEM_ALLOC,str_cic[i],sizeof(mem_cic));        
    rt_free(MEM_ALLOC,str_demod[i]->symbLock,nSymb*sizeof(int));
    rt_free(MEM_ALLOC,str_demod[i]->symbOut,nSymb*sizeof(int));
    rt_free(MEM_ALLOC,str_demod[i],sizeof(demod_mem));    
  }

  printf("---------------> Check <-------------------\n\n");  
  return 0;
}

