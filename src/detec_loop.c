#include "detect_loop.h"
#define N	2048
uint32_t DDS_Detection_Loop(signal) {

	int32_t i, currIdx = 0, lower_limit = 0, upper_limit = 0;
	uint8_t ret_value = DDS_INSERT_FREQ_NONE, aux_abs, isInPrevIdx;
	uint32_t peakAmp = 1, peakIdx = 0, currAmp = 0, iPass=0, peakPos, iPrevIdx, insertedFreqs[DDS_NUMBER_OF_DECODERS];

	uint32_t tmp0;
	uint8_t nPass=0, assigned_decoder = 0;
	DDS_PassSet_Typedef * passSet;
	DDS_FreqsRecord_Typedef DDS_PTT_DP_LIST[DDS_NUMBER_OF_DECODERS];

	/*Number_of_detected_indexes()
	* In this loop, passSet stores the uFFT response.
	* can find the signal frequency through the fft index 
	* through the equation: f = idx*fs/L, where fs is sampling frequency and
	* M is the lentgh of FFT.
	*/
	//TODO resetPassSet();
	ufft(signal,N);
	calc_mask();
	for(i = 0; i<N; i++){
		if(signal(i)>mask(i)){
			passSet->passIdx = i;
			passSet->passIdx = signal(i);
			nPass++;
		}
	}

	while(peakAmp!=0 && *assigned_decoder!=DDS_INSERT_FREQ_INVALID){
		*assigned_decoder = DDS_INSERT_FREQ_INVALID;
		peakAmp = 0;

		//Loop for find decoder free
		for (i = 0; i < DDS_NUMBER_OF_DECODERS; i++){
			if(DDS_PTT_DP_LIST[i].detect_state==FREQ_NONE){
				*assigned_decoder=i;
				break;
			}
		}
		//If anyone decoder is available:
		while(*assigned_decoder != DDS_INSERT_FREQ_INVALID && iPass<nPass){
			currIdx = (int32_t) passSet->passIdx[iPass];
			currAmp = passSet->passAmp[iPass];
			if (TEST_SIGN_BIT(currIdx, DDS_FREQ_NUMBER_OF_BITS)) {
				currIdx = CONVERT_TO_32BIT_SIGNED(currIdx,
				DDS_FREQ_NUMBER_OF_BITS); // sign extent
				// now currIdx is in 2's complement 32bit
			}
			//first amplitude test, find the highest amplitude signal.
			if (currAmp > peakAmp) {
				isInPrevIdx = 0;
				//Test if signal is present in two consecutive windows (double detection criteria)
				//The loop scan the signal vector of the previous window 
				for (iPrevIdx = 0; iPrevIdx<nPrevIdx; iPrevIdx++){
					if(currIdx==prevIdx[iPrevIdx]){ //doubleDETECTION
						isInPrevIdx = 1;
						break;
					}
				}
				//Assign the signal that passes as a peak
				if(isInPrevIdx){
					//FREQ_DETECTED_TWICE;
					peakAmp = currAmp;
					peakIdx = currIdx;
					peakPos = iPass;
				} else{
						/*
					* FREQ_DETECTED_ONCE;
					* DDS_PTT_DP_LIST[*assigned_decoder].detect_state = DDS_INSERT_FREQ_NONE;
					*/
					}
			}
			iPass++;
		}
/*
		* Update Detected PttDpList
*/
		if(peakAmp>0){
			DDS_PTT_DP_LIST[*assigned_decoder].freq_idx = passSet->passIdx[peakPos];
			DDS_PTT_DP_LIST[*assigned_decoder].freq_amp = peakAmp;
			//Update passSet
			passSet->passAmp[peakPos]=0;
			passSet->passIdx[peakPos]=0;
			ret_value = DDS_INSERT_FREQ_NEW;
		}
	}
	//Update prevPassIdx
	for (iPass=0;iPass<nPass;iPass++){
		currIdx = (int32_t) passSet->passIdx[iPass];
		if (TEST_SIGN_BIT(currIdx, DDS_FREQ_NUMBER_OF_BITS)) {
			currIdx = CONVERT_TO_32BIT_SIGNED(currIdx,
			DDS_FREQ_NUMBER_OF_BITS); // sign extent
		}
		prevIdx[iPass]=currIdx;
	}
	//The length of previows indexes in next window is length of current window
	nPrevIdx = nPass; 
	return ret_value;
}


//mask
void calc_mask(*mask){
	uint32_t hat_f, hat_a;
	
	for (i = 0; i < DDS_NUMBER_OF_DECODERS; i++){
		if(DDS_PTT_DP_LIST[i].detect_state!= DECOD_FREE){
			hat_f = DDS_PTT_DP_LIST[i].freq_idx;
			hat_a = DDS_PTT_DP_LIST[i].freq_amp;

			hat_f_left = (hat_f<52)? (hat_f+2047-52):hat_f;
			hat_f_right = (hat_f>2047-52)? (hat_f+52-2047):hat_f;

			for (i = hat_f_left-52; i <= hat_f_right+52; i++){
				if(abs(i-hat_f)<=26 && mask(i)<2*hat_a){ //represents a band of 3.2kHz
					mask(i) = 2*hat_a;
				}else if(abs(i-hat_f)<=38 && mask(i)<hat_a/6){ 
					mask(i) = hat_a/6;
				}else if(abs(i-hat_f)<=52 && mask(i)<3*hat_a/32){
					mask(i) = 3*hat_a/32;
				}

			}			
			
		}
	}
}
