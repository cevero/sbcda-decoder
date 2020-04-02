#include "service.h"
unsigned int patternA, patternB;
#define MAX_SYMBOLS_PER_WINDOW 8
#define MAX_SYNCH_SYMBOLS 64
#define SYNCH_MIN_MASK 0x3FF
#define SYNCH_PATTERN	0xFFFE2F
void frameSynch(PTTPackage_Typedef * wpckg, unsigned int pttd_symbol)
{
	if (wpckg->total_symbol_cnt > MAX_SYNCH_SYMBOLS) { // synch failed
		wpckg->synch_state = PTT_SYNCH_FAILED;
		wpckg->status = PTT_ERROR;
	} else {
		wpckg->symb_array[0] = wpckg->symb_array[1]; // previous
		wpckg->symb_array[1] = pttd_symbol; // current
		
		patternA = wpckg->synch_patternA;
		patternB = wpckg->synch_patternB;
		unsigned int decoded_bit = decodeBit(wpckg->symb_array[0], wpckg->symb_array[1]);
		if (wpckg->total_symbol_cnt >= 2){
			if ((wpckg->total_symbol_cnt % 2) == 0) {
				wpckg->synch_patternA = (wpckg->synch_patternA) << 1;
				if (decoded_bit) { // bit 1 detected
					wpckg->synch_patternA = (wpckg->synch_patternA) | 0x0001;
				}
			} else {
				wpckg->synch_patternB = (wpckg->synch_patternB) << 1;
				if (decoded_bit) { // bit 1 detected
					wpckg->synch_patternB = (wpckg->synch_patternB) | 0x0001;
				}
			}
			if ((wpckg->synch_patternA & SYNCH_MIN_MASK)
					== (SYNCH_PATTERN & SYNCH_MIN_MASK)
					|| (wpckg->synch_patternB & SYNCH_MIN_MASK)
							== (SYNCH_PATTERN & SYNCH_MIN_MASK)) { // match!!!
				wpckg->status = PTT_DATA;
				wpckg->synch_state = PTT_SYNCH_OK;
				wpckg->symb_cnt = 0;
			}
		}
	}
}