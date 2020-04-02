#include "service.h"
#define MAX_SYMBOLS_PER_WINDOW 8
void readData(PTTPackage_Typedef * wpckg, unsigned int pttd_symbol) 
{
	unsigned int i0;
	unsigned int msgBitLength;
	unsigned int tmp0;
	_Bool userData;

	if (wpckg->symb_cnt < MAX_SYMBOLS_PER_WINDOW) {
		// just accumulate 8 symbols for now
		wpckg->symb_array[wpckg->symb_cnt++] = pttd_symbol;
	}
	// all message length symbols were received
	if (wpckg->symb_cnt == MAX_SYMBOLS_PER_WINDOW) {
		for (i0 = 0; i0 < MAX_SYMBOLS_PER_WINDOW; i0 += 2) {
			// bit_cnt>>3 == bit_cnt / 8 => gives the byte position on the stream
			tmp0 = wpckg->userMsg[(wpckg->bit_cnt) >> 3];
			tmp0 <<= 1;
			if (decodeBit(wpckg->symb_array[i0],
					wpckg->symb_array[i0 + 1])) { // bit 1
				tmp0 = tmp0 | 0x01;
			}
			wpckg->userMsg[(wpckg->bit_cnt) >> 3] = tmp0;

			wpckg->bit_cnt++;
		}
		wpckg->symb_cnt = 0;
	}
	if(wpckg->bit_cnt>0 && wpckg->bit_cnt<=4){
		wpckg->errorCode ^= ((wpckg->userMsg[0]>>(wpckg->bit_cnt-1))&0x1);
	}
	if(wpckg->bit_cnt==4){
		msgBitLength = calcMessageLength(wpckg->userMsg[0]&0xF);
		wpckg->msgByteLength = msgBitLength>>3;
	}
	userData = (wpckg->bit_cnt > 4); //Next msgLength store ID and userData
	if (wpckg->bit_cnt >= (wpckg->msgByteLength<<3) && userData) {
		wpckg->status = PTT_READY;
	}
}