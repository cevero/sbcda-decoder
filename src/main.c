#include "decoder.h"
#include <stdlib.h>
#include <omp.h>
#include <malloc.h>

#define NUMBER_OF_SAMPLES 153600//140800

int main(int argc, char * argv[]){
  if(argc !=2)
  {
      printf("use ./main <nthreads>\n");
      exit(0);
  }
  
  double startParallel,finishParallel, sumParallelTime = 0.0;


  int n0;
  int nthreads = atoi(argv[1]);
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
  sampler_mem *str_smp[NUMBER_OF_DECODERS];
  int n_decod = NUMBER_OF_DECODERS;
printf("debug: %d",smplPerSymb);

  for (i = 0; i < n_decod; ++i)
  {
      str_smp[i] = malloc (sizeof(sampler_mem));
      str_smp[i]->smplBuffer = calloc(2,smplPerSymb*(sizeof(int)));
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
  int tmp0=0,f=0,decoder_index,vga, activeList, n, i0,i1,i2;
  int vgaExp[NUMBER_OF_DECODERS],vgaMant[NUMBER_OF_DECODERS],InitFreq[NUMBER_OF_DECODERS];

  for(i=0;i<NUMBER_OF_DECODERS;i++){
    printf("Clearing decoder %d\n",i);
    clearDecoder(PTT_DP_LIST[i],wpckg[i], str_cic[i], str_cicSmp[i], str_smp[i], str_demod[i]);
  }
  
  #define NSIM (150)
  
  for(int n0; n0<NSIM;n0++)
  {

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
    
    for (decoder_index=0;decoder_index<NUMBER_OF_DECODERS;decoder_index++){
      
      if(PTT_DP_LIST[decoder_index]->detect_state==FREQ_DETECTED_TWICE){
        vga = VgaGain(PTT_DP_LIST[decoder_index]->freq_amp);
        vgaExp[decoder_index] = -1*(vga&0x3F);
        vgaMant[decoder_index] = (vga>>6)&0xFF;
        InitFreq[decoder_index] = PTT_DP_LIST[decoder_index]->freq_idx<<9;
        printf("[%d]: mant %d exp %d\n",decoder_index, vgaMant[decoder_index],vgaExp[decoder_index]);
        PTT_DP_LIST[decoder_index]->detect_state=FREQ_DECODING;
        wpckg[decoder_index]->status=PTT_FRAME_SYNCH;
      }
    }
    
    startParallel = omp_get_wtime();
    //decodes signals from active channels

#pragma omp parallel for default (shared) private(decoder_index) num_threads(nthreads)

    for (decoder_index=0;decoder_index<NUMBER_OF_DECODERS;decoder_index++)
    {
      if(PTT_DP_LIST[decoder_index]->detect_state==FREQ_DECODING)
      {
#pragma omp critical
        pttA2Demod(inputSignal, InitFreq[decoder_index], vgaMant[decoder_index],vgaExp[decoder_index],
                str_demod[decoder_index], str_cic[decoder_index], str_cicSmp[decoder_index], str_smp[decoder_index]);

        for(i1 = 0;i1<nSymb;i1++)
        {
            if(str_demod[decoder_index]->symbLock[i1])
            {
            wpckg[decoder_index]->total_symbol_cnt++;

                if(wpckg[decoder_index]->status==PTT_FRAME_SYNCH)
                {
                  frameSynch(wpckg[decoder_index],str_demod[decoder_index]->symbOut[i1]);   
                }
                else if(wpckg[decoder_index]->status==PTT_DATA)
                {
                  readData(wpckg[decoder_index],str_demod[decoder_index]->symbOut[i1]);

                  if(wpckg[decoder_index]->status==PTT_READY)
                  {
                    //fill the package and clear the decoder
                    printf("ready!\n");
                    printf("|%d|\n",decoder_index);
                    for(i2=0;i2<wpckg[decoder_index]->msgByteLength;i2++)
                    {
                      printf("%x\n",wpckg[decoder_index]->userMsg[i2]);
                    }
                    printf("Clearing decoder %d\n",decoder_index);
                    clearDecoder(PTT_DP_LIST[decoder_index],wpckg[decoder_index], str_cic[decoder_index], str_cicSmp[decoder_index], str_smp[decoder_index], str_demod[decoder_index]);
                    //DEBUG Purpose
                  }
                }
                else if(wpckg[decoder_index]->status==PTT_ERROR)
                {
                  printf("Clearing decoder %d\n",decoder_index);
                  clearDecoder(PTT_DP_LIST[decoder_index],wpckg[decoder_index], str_cic[decoder_index], str_cicSmp[decoder_index], str_smp[decoder_index], str_demod[decoder_index]);
                }
            }
        }
      }
    }
/*end of decode for loop*/
    
    finishParallel = omp_get_wtime();
    sumParallelTime+=finishParallel-startParallel;
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


  FILE *output_time;
  output_time = fopen("parallelTime.txt","a");
  fprintf(output_time,"nthreads %d time = %f\n",nthreads,sumParallelTime);
  printf("with %d threads time for parallel zone is = %f\n",nthreads,sumParallelTime);
  printf("---------------> Check <-------------------\n\n");  
  return 0;
}

