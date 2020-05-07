#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <complex.h>
#include "fft.h"
#include "detect_loop.h"

int prevIdx[] = {[0 ... DFT_LENGTH-1]=0};
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

void calc_mask(int * mask, FreqsRecord_Typedef *PTT_DP_LIST)
{
  int hat_f, hat_f_left, hat_f_right, hat_a, mask_cnt;
  int i,i0,state,L1,L2,L3;
  int gBand = 52, cnt_band;

  for (i = 0; i < NUMBER_OF_DECODERS; i++){
    if(PTT_DP_LIST[i].detect_state != FREQ_NONE){
      hat_f = PTT_DP_LIST[i].freq_idx;
      hat_a = PTT_DP_LIST[i].freq_amp;
    
            state = 0;
            cnt_band = 0;
            L1 = 3*hat_a/32;
            L2 = hat_a/6;
            L3 = 2*hat_a;

      hat_f_left = (hat_f<52)? (hat_f+2048-52):(hat_f-52);
      hat_f_right = (hat_f>2047-52)? (hat_f+52-2048):(hat_f+52);
      mask_cnt = hat_f_left;

      while(mask_cnt!=hat_f_right+1){
                if (mask_cnt == 2048){
                    mask_cnt=0;
                }

        if(state==0){
                    state = (cnt_band==13)? 1:0;
                    if(mask[mask_cnt] < L1){
                        mask[mask_cnt] = L1;
                    }
                }else if(state==1){
                    state = (cnt_band==25)? 2:1;
                    if(mask[mask_cnt] < L2){
                        mask[mask_cnt] =  L2;
                    }
                }else if(state==2){
                    state = (cnt_band==78)? 3:2;
          if(mask[mask_cnt] < L3){
            mask[mask_cnt] = L3;//26 represents a band of 3.2kHz
          }
                }else if(state==3){
                    state = (cnt_band==90)? 0:3;
                    if(mask[mask_cnt]<L2){
                        mask[mask_cnt] = L2;
                    }
                }
        cnt_band++;

//        mask[mask_cnt] = 0xFFFFFFFF;
        mask_cnt++;
            }
    }
  }
}

unsigned int detectLoop(int complex *inputSignal, FreqsRecord_Typedef *PTT_DP_LIST)
{

	int i, currIdx = 0;
    /*int lower_limit = 0, upper_limit = 0;*/
	unsigned int ret_value = FREQ_NONE, isInPrevIdx;
	unsigned int peakAmp = 1, currAmp = 0, iPass=0, peakPos, iPrevIdx;
  int mask[] = {[0 ... DFT_LENGTH-1] = DEFAULT_AMP_THRESHOLD};
	//float complex fftSignal[] = {[0 ... DFT_LENGTH-1] = 0+0*I};
	float complex *fftSignal = malloc(DFT_LENGTH*sizeof(int complex));
    //divisibiNlity fixed

	/*unsigned int tmp0, peakIdx = 0, aux_abs, insertedFreqs[DDS_NUMBER_OF_DECODERS];*/
	unsigned int nPass=0, assigned_decoder = 0;
    /*save the window of signal in freq*/
    PassSet_Typedef passSet;

	/*Number_of_detected_indexes()
	* In this loop, passSet stores the uFFT response.
	* can find the signal frequency through the fft index 
	* through the equation: f = idx*fs/L, where fs is sampling frequency and
	* M is the length of FFT.
	*/
	
		for (int n = 0; n < DFT_LENGTH; n++) {
      if (n<WINDOW_LENGTH) {
        fftSignal[n] = inputSignal[n];
      } else {
        fftSignal[n] = 0;
      }
    }
		//printf("here! %d\n",DFT_LENGTH);		
    fft(fftSignal,DFT_LENGTH);		
		
    calc_mask(mask,PTT_DP_LIST);
		
    /* Compare fft amplitude with mask */
    for(i = 0; i<DFT_LENGTH; i++){
        //printf("[%d]: abs signal: %f, mask: %d\n", i, cabs(signal[i]), mask[i]);
				fftSignal[i] = fftSignal[i]/(2048);//2048/1.6 keep Vga values
        if(cabs(fftSignal[i])>mask[i]){
            /*printf("abs signal: %f, mask: %d\n", cabs(signal[i]), mask[i]);*/
            passSet.Idx[nPass] = i;
            passSet.Amp[nPass] = cabs(fftSignal[i]);
            nPass++;
        }
    }
		free(fftSignal);
    //printf("nPass: %d\n",nPass);

    while(peakAmp > 0 && assigned_decoder != FREQ_INVALID){

        assigned_decoder = FREQ_INVALID;
        //Loop for find decoder free
        for (i = 0; i < NUMBER_OF_DECODERS; i++){
            if(PTT_DP_LIST[i].detect_state == FREQ_NONE){
                //printf("Decoder free %d\n",i);
                assigned_decoder=i;
                break;
            }
        }
				
        calc_mask(mask,PTT_DP_LIST);

        peakAmp = 0;iPass = 0;
        while(assigned_decoder != FREQ_INVALID && iPass<nPass){
            currIdx = (int) passSet.Idx[iPass];
            currAmp = mask[currIdx];            
            //printf("currIdx: %d - currAmp: %d, iPass: %d\n",passSet.Idx[iPass],passSet.Amp[iPass],iPass);
           if(passSet.Amp[iPass]<currAmp){ //Compute mask
                passSet.Amp[iPass]=0;
            }else{
                currAmp = passSet.Amp[iPass];
                //printf("currIdx: %d - currAmp: %d, iPass: %d\n",passSet.Idx[iPass],passSet.Amp[iPass],iPass);
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
            PTT_DP_LIST[assigned_decoder].freq_idx
                = passSet.Idx[peakPos];
            PTT_DP_LIST[assigned_decoder].freq_amp = peakAmp;
            PTT_DP_LIST[assigned_decoder].detect_state = FREQ_DETECTED_TWICE;
						PTT_DP_LIST[assigned_decoder].timeout = DEFAULT_TIMEOUT;
						//printf("freq detected: %d\n AMP: %d \n",passSet.Idx[peakPos], peakAmp);
            ret_value = 1;
        }
				
    } //while
   //Update prevPassIdx
    memset(prevIdx, 0, sizeof(prevIdx));
		
  for (iPass=0; iPass < nPass; iPass++){
    prevIdx[passSet.Idx[iPass]]=1;
  }		
  return ret_value;
}