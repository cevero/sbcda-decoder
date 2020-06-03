#include "decoder.h"
#include <omp.h>

#define NUMBER_OF_SAMPLES 153600//140800

int main(){
  int n0;
  printf("--------> Loading Input Signal <-------\n\n");
  FILE* inputFile = fopen("inputSignal12.txt","r");

  if(inputFile == NULL){
    printf("Can't open file!\n");
    return 1;
  }
  char input[100];
  int inputRe[NUMBER_OF_SAMPLES];
  int inputIm[NUMBER_OF_SAMPLES];

  for (n0=0;fgets(input,sizeof(input), inputFile) != NULL;n0++){
    sscanf(input,"%d %d",&inputRe[n0], &inputIm[n0]);
  }
  fclose(inputFile);

  printf("--------> Finish Load Input Signal <-------\n\n");   
    
  /********************************************************************************/
 
  //sampler memory
  int i;
  sampler_mem * str_smp[NUMBER_OF_DECODERS];
  for (i = 0; i < NUMBER_OF_DECODERS; ++i){
    str_smp[i] = malloc(sizeof(sampler_mem));
    str_smp[i]->smplBuffer = malloc(2*smplPerSymb*(sizeof(int)));
  }

  //CIC filter of demod
  mem_cic * str_cic[NUMBER_OF_DECODERS];
  for (i = 0; i < NUMBER_OF_DECODERS; ++i){
    str_cic[i] = malloc(sizeof(mem_cic));
    str_cic[i]->previousAccRe = malloc(delayIdx*sizeof(int));
    str_cic[i]->previousAccIm = malloc(delayIdx*sizeof(int));
  }
  //CIC filter of sampler
  mem_cic * str_cicSmp[NUMBER_OF_DECODERS];
  for (i = 0; i < NUMBER_OF_DECODERS; ++i){
    str_cicSmp[i]= malloc(sizeof(mem_cic));
    str_cicSmp[i]->previousAccRe = malloc(delaySmp*sizeof(int));
    str_cicSmp[i]->previousAccIm = malloc(delaySmp*sizeof(int));
  }

  //demod_mem stores accumulators, symbOut and symbLock
  demod_mem * str_demod[NUMBER_OF_DECODERS];
  for(i=0;i<NUMBER_OF_DECODERS;i++){
    str_demod[i] = malloc(sizeof(demod_mem));
    str_demod[i]->symbLock = malloc(nSymb*sizeof(int));
    str_demod[i]->symbOut = malloc(nSymb*sizeof(int));
  }
  
  // struct stores the interface (detect to demod) parameters
  FreqsRecord_Typedef *PTT_DP_LIST[NUMBER_OF_DECODERS];
  for(i=0;i<NUMBER_OF_DECODERS;i++){
    PTT_DP_LIST[i] = malloc(sizeof(FreqsRecord_Typedef));
  }
  
  // This struct is used to control the FSM and show bit results
  PTTPackage_Typedef * wpckg[NUMBER_OF_DECODERS];
  for(i=0;i<NUMBER_OF_DECODERS;i++){
    wpckg[i] = malloc(sizeof(PTTPackage_Typedef));
  }


  printf("***------------ ALL READY --------------***\n");
  int complex *inputSignal = malloc(WINDOW_LENGTH*sizeof(int complex));
  int tmp0=0,f=0,iCh,vga, activeList, n, i0,i1,i2;
  int vgaExp[NUMBER_OF_DECODERS],vgaMant[NUMBER_OF_DECODERS],InitFreq[NUMBER_OF_DECODERS];

  for(i=0;i<NUMBER_OF_DECODERS;i++){
    printf("Clearing decoder %d\n",i);
    clearDecoder(PTT_DP_LIST[i],wpckg[i], str_cic[i], str_cicSmp[i], str_smp[i], str_demod[i]);
  }
  
  #define NSIM (150)
  
  for(int n0; n0<NSIM;n0++){
  for (i0=0;i0<NUMBER_OF_SAMPLES/WINDOW_LENGTH;i0++){
    printf("***--------- Window processing ---------*** [%d]\n", i0);
    // Performs input partitioning on the windows of 1280 samples
     
    for (n = 0; n < WINDOW_LENGTH; n++) {
      inputSignal[n] = inputRe[WINDOW_LENGTH*i0+n]+inputIm[WINDOW_LENGTH*i0+n]*I;
    }

    UpdateTimeout(PTT_DP_LIST,wpckg);

    tmp0 =  detectLoop(inputSignal, PTT_DP_LIST);
    
    //DEBUG Purpose
     if(tmp0){
      printf("New PTT(s) detected!\nStatus of all decoders:\n");
      for(n=0;n<NUMBER_OF_DECODERS;n++){
        printf("ch:%d state: %d freq: %4d amp: %4d\n",n,PTT_DP_LIST[n]->detect_state, PTT_DP_LIST[n]->freq_idx, PTT_DP_LIST[n]->freq_amp);
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
        printf("[%d]: mant %d exp %d\n",iCh, vgaMant[iCh],vgaExp[iCh]);
        PTT_DP_LIST[iCh]->detect_state=FREQ_DECODING;
        wpckg[iCh]->status=PTT_FRAME_SYNCH;
      }
    }
    
    //decodes signals from active channels
//#pragma omp parallel for default (none) shared(str_smp,str_cic,str_demod,vgaMant,vgaExp,PTT_DP_LIST,InitFreq,str_cicSmp,i1,i2,wpckg) private(inputSignal)
    for (iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
      if(PTT_DP_LIST[iCh]->detect_state==FREQ_DECODING){
        pttA2Demod(inputSignal, InitFreq[iCh], vgaMant[iCh],
        vgaExp[iCh], str_demod[iCh], str_cic[iCh], str_cicSmp[iCh], str_smp[iCh]);
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
      }
    }
    
  }
  }
    
  free(inputSignal);
  for(i=0;i<NUMBER_OF_DECODERS;i++){
    free(PTT_DP_LIST[i]);
    free(str_smp[i]->smplBuffer);   
    free(str_smp[i]);       
    free(str_cicSmp[i]->previousAccRe);
    free(str_cicSmp[i]->previousAccIm);
    free(str_cicSmp[i]);
    free(str_cic[i]->previousAccRe);
    free(str_cic[i]->previousAccIm);
    free(str_cic[i]);        
    free(str_demod[i]->symbLock);
    free(str_demod[i]->symbOut);
    free(str_demod[i]);    
  }
  printf("---------------> Check <-------------------\n\n");  
  return 0;
}

