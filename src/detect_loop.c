#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <complex.h>
#include "detect_loop.h"
#include "service.h"
#include "rt/rt_api.h"

//int prevIdx[] = {[0 ... DFT_LENGTH-1]=0};
//FreqsRecord_Typedef PTT_DP_LIST[NUMBER_OF_DECODERS];
/* DDS_Mask
*
* This function calculates the detection mask that
* must be applied when analyzing the insertion of
* new frequencies into the decoder.
*
* The amplitude of the mask is calculated according
* to the frequency (hat_f) and amplitude (â) of a signal
* detected twice or in decoding state.
* For bins with a bandwidth of 3.2 kHz around the hat_f
* the amplitude of the mask will be twice the â.
* For bins with a bandwith around the hat_f of 4.8 kHz
* the amplitude of the mask will be 1/6*â.
* For bins with a bandwith of 6.4 kHz around the hat_f
* the amplitude of the mask will be 3*â/32.
*
*/

void calc_mask(int * mask, FreqsRecord_Typedef * PTT_DP_LIST[NUMBER_OF_DECODERS])
{
  int hat_f, hat_f_left, hat_f_right, hat_a, mask_cnt;
  int i,i0;
  int gBand = 52;

  for (i = 0; i < NUMBER_OF_DECODERS; i++){
    if(PTT_DP_LIST[i]->detect_state != FREQ_NONE){
      hat_f = PTT_DP_LIST[i]->freq_idx;
      hat_a = PTT_DP_LIST[i]->freq_amp;
    
      hat_f_left = (hat_f<52)? (hat_f+2048-52):(hat_f-52);
      hat_f_right = (hat_f>2047-52)? (hat_f+52-2048):(hat_f+52);
      mask_cnt = hat_f_left;

      while(mask_cnt!=hat_f_right+1){
        if (mask_cnt == 2048){
            mask_cnt=0;
        }
        mask[mask_cnt] = 0xFFFF;
        mask_cnt++;
      }
    }
  }
}
//#define microFFT
//#define kissFFT
unsigned int detectLoop(int complex *inputSignal,cpx * we,int * prevIdx, FreqsRecord_Typedef * PTT_DP_LIST[NUMBER_OF_DECODERS])
{

  int i, n, currIdx = 0;
    /*int lower_limit = 0, upper_limit = 0;*/
  unsigned int ret_value = FREQ_NONE, isInPrevIdx;
  unsigned int peakAmp = 1, currAmp = 0, iPass=0, peakPos, iPrevIdx;
  int * mask = rt_alloc(RT_ALLOC_FC_RET_DATA,DFT_LENGTH*sizeof(int));
//  int mask;
//  float complex * fftSignal = rt_alloc(RT_ALLOC_FC_RET_DATA,DFT_LENGTH*sizeof(float complex));
//  float complex * scratch = rt_alloc(RT_ALLOC_FC_RET_DATA,DFT_LENGTH*sizeof(float complex));
//  int fftSignal;
//  kiss_fft_cpx *fftInput = rt_alloc(RT_ALLOC_FC_RET_DATA,DFT_LENGTH*sizeof(kiss_fft_cpx));
//  kiss_fft_cpx *fftOutput = rt_alloc(RT_ALLOC_FC_RET_DATA,DFT_LENGTH*sizeof(kiss_fft_cpx));
//  kiss_fft_cfg cfg;
  cpx * fftSignal = rt_alloc(RT_ALLOC_FC_RET_DATA,DFT_LENGTH*sizeof(cpx));
  int nPass=0;
  int assigned_decoder = 0;
  /*save the window of signal in freq*/
  PassSet_Typedef * passSet = rt_alloc(RT_ALLOC_FC_RET_DATA,sizeof(PassSet_Typedef));
//  cfg = kiss_fft_alloc(DFT_LENGTH,0,NULL,NULL);
  
  memset(passSet->Idx,0,sizeof(passSet->Idx));
  memset(passSet->Amp,0,sizeof(passSet->Amp));
 
  /*Number_of_detected_indexes()
  * In this loop, passSet stores the uFFT response.
  * can find the signal frequency through the fft index 
  * through the equation: f = idx*fs/L, where fs is sampling frequency and
  * M is the length of FFT.
  */

  
  for (n = 0; n < DFT_LENGTH; n++) {
    mask[n] = 500;
    if (n<WINDOW_LENGTH) {
//      fftSignal[n] = inputSignal[n];
      fftSignal[n].r = (int) creal(inputSignal[n]);
      fftSignal[n].i = (int) cimag(inputSignal[n]);
//      fftInput[n].r = (int) creal(inputSignal[n]);
//      fftInput[n].i = (int) cimag(inputSignal[n]);
    } else {
      fftSignal[n].r = 0;
      fftSignal[n].i = 0;
//      fftInput[n].r = (int) 0;
//      fftInput[n].i = (int) 0;
    }
  }
  //printf("here! %d %d\n",fftSignal[0].r,fftSignal[0].i);

  //fft(scratch,fftSignal,DFT_LENGTH);
  fft_itO(fftSignal,we,DFT_LENGTH);
  //kiss_fft(cfg,fftInput,fftOutput);
  calc_mask(mask,PTT_DP_LIST);
  int aux;
  // Compare fft amplitude with mask 
  for(i = 0; i<DFT_LENGTH; i++){
  //      printf("[%d]: abs signal: %f, mask: %d\n", i, cabs(fftSignal[i]), mask[i]);
  //      fftSignal[i] = fftSignal[i]/(2048);//2048/1.6 keep Vga values
        aux = (int)(sqrt(fftSignal[i].r*fftSignal[i].r+fftSignal[i].i*fftSignal[i].i))/2048;
//        printf("[%d]: abs signal: %f, mask: %d\n", i, aux, mask[i]);
        if(aux>mask[i]){
//            printf("[%d]: %d, mask: %d\n", i,aux, mask[i]);
            passSet->Idx[nPass] = i;
            passSet->Amp[nPass] =(int) aux;
  //          printf("[*]: %d, nPass: %d\n", passSet->Amp[nPass], nPass);
            nPass++;
      }
  }
  //	printf("nPass: %d\n",nPass);
    
    while(peakAmp > 0 && assigned_decoder != FREQ_INVALID){
        assigned_decoder = FREQ_INVALID;
        //Loop for find decoder free
        for (i = 0; i < NUMBER_OF_DECODERS; i++){
            if(PTT_DP_LIST[i]->detect_state == FREQ_NONE){
  //             printf("Decoder free %d\n",i);
               assigned_decoder=i;
               break;
            }
        }
        
        //printf("assigned_decoder %d\n",assigned_decoder);
        calc_mask(mask,PTT_DP_LIST);

        peakAmp = 0;iPass = 0;
	
        while(assigned_decoder != FREQ_INVALID && iPass<nPass){
            currIdx = (int) passSet->Idx[iPass];
            currAmp = mask[currIdx];            
            //printf("currIdx: %d - currAmp: %d, iPass: %d\n",passSet.Idx[iPass],passSet.Amp[iPass],iPass);
           if(passSet->Amp[iPass]<currAmp){ //Compute mask
                passSet->Amp[iPass]=0;
            }else{
                currAmp = passSet->Amp[iPass];
		//printf("currIdx: %d - currAmp: %d, iPass: %d\n",passSet->Idx[iPass],passSet->Amp[iPass],iPass);
                //first amplitude test, find the highest amplitude signal.
                if (currAmp > peakAmp) {
                    //Test if signal is present in two consecutive windows
                    //isInPrevIdx = prevIdx[currIdx];
                    if(prevIdx[currIdx]){//FREQ_DETECTED_TWICE//
                        //Assign the signal that passes as a peak
                        peakAmp = currAmp;
                        //peakIdx = currIdx;
                        peakPos = iPass;
                    }
                }
            }
          iPass++;
        }
     //Update Detected PttDpList
        if(peakAmp>0){
//            printf("assigned_decoder %d\n",assigned_decoder);
            PTT_DP_LIST[assigned_decoder]->freq_idx
                = passSet->Idx[peakPos];
            PTT_DP_LIST[assigned_decoder]->freq_amp = peakAmp;
            PTT_DP_LIST[assigned_decoder]->detect_state = FREQ_DETECTED_TWICE;
            PTT_DP_LIST[assigned_decoder]->timeout = DEFAULT_TIMEOUT;
//            printf("freq detected: %d\n AMP: %d \n",passSet->Idx[peakPos], peakAmp);
            ret_value = 1;
        }    
    } //while
   
   //Update prevPassIdx
    memset(prevIdx, 0, sizeof(prevIdx));
    
  for (iPass=0; iPass < nPass; iPass++){
    prevIdx[passSet->Idx[iPass]]=1;
  }
 
  rt_free(RT_ALLOC_FC_RET_DATA,fftSignal,DFT_LENGTH*sizeof(cpx));
  rt_free(RT_ALLOC_FC_RET_DATA,passSet,sizeof(PassSet_Typedef));
//  rt_free(RT_ALLOC_FC_RET_DATA,fftInput,DFT_LENGTH*sizeof(kiss_fft_cpx));
//  rt_free(RT_ALLOC_FC_RET_DATA,fftOutput,DFT_LENGTH*sizeof(kiss_fft_cpx));
  rt_free(RT_ALLOC_FC_RET_DATA,mask,DFT_LENGTH*sizeof(int));
  return ret_value;
}
