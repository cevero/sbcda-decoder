#include "detect_loop.h"

uint32_t __DDS_Detection_Loop() {

	int32_t i, currIdx = 0, idx_min_distance = 51,
			lower_limit = 0, upper_limit = 0;
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
	* L is the lentgh of FFT.
	*while(!DDS_DetectEop() || !DDS_DetectEmpty()){
	*	tmp0 = EDC_DECODER->detectBufferData;
	*	passSet->passIdx[nPass] = DDS_DETECTDATA_DETECT_FREQ_VALUE(tmp0);
	*	passSet->passAmp[nPass] = DDS_DETECTDATA_DETECT_AMP_VALUE(tmp0);
	*	nPass++;
	*}
	*/


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
		//If decoder is available:
		while(*assigned_decoder != DDS_INSERT_FREQ_INVALID && iPass<nPass){
			//for (iPass = 0; iPass < nPass; iPass++) {
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
			* BandGuard test
			* Update Detected PttDpList
			* TODO implemented the mask
*/

			if(peakAmp>0){
				DDS_PTT_DP_LIST[*assigned_decoder].detect_state = FREQ_DETECTED_TWICE;
				for (i = 0; i < DDS_NUMBER_OF_DECODERS; i++){
					if(DDS_PTT_DP_LIST[i].detect_state != FREQ_NONE){
						currIdx=DDS_PTT_DP_LIST[i].freq_idx;
						if (TEST_SIGN_BIT(currIdx,DDS_FREQ_NUMBER_OF_BITS)){
							currIdx = CONVERT_TO_32BIT_SIGNED(currIdx,
									DDS_FREQ_NUMBER_OF_BITS);
						}
						//aux_abs for test absolute distance between signals in frequency
						aux_abs = currIdx<peakIdx;
						lower_limit = ((currIdx - idx_min_distance) < -1024)? -1024:(currIdx - idx_min_distance);
						upper_limit = ((currIdx + idx_min_distance) > 1023)? 1023:(currIdx + idx_min_distance);
						if((!aux_abs&(peakIdx >= lower_limit)) || ((peakIdx<= upper_limit)&aux_abs)) {
							// The detected signal is too close to a decoding signal
							DDS_PTT_DP_LIST[*assigned_decoder].detect_state = DDS_INSERT_FREQ_NONE;
							//Update passSet
							passSet->passAmp[peakPos]=0;
							passSet->passIdx[peakPos]=0;
							break;
						}
					}
				}
				if(DDS_PTT_DP_LIST[*assigned_decoder].detect_state == FREQ_DETECTED_TWICE){
					DDS_PTT_DP_LIST[*assigned_decoder].freq_idx = passSet->passIdx[peakPos];
					DDS_PTT_DP_LIST[*assigned_decoder].freq_amp = peakAmp;
					//Update passSet
					passSet->passAmp[peakPos]=0;
					passSet->passIdx[peakPos]=0;
					ret_value = DDS_INSERT_FREQ_NEW;
				}
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