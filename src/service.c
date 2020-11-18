#include "service.h"

void incAndOvFlow(int prevAcc,int inc,int nInc,int accWidth, int * accOut)
{
	int acc = prevAcc;
	int upLimit = pow(2,accWidth);
	int iSmpl;

	for (iSmpl=0;iSmpl<nInc;iSmpl++){
	  acc = acc + inc;
	  if (acc >= upLimit){
	    acc = acc-upLimit;
	  }else if (acc < 0){
	    acc = acc+upLimit;
	  }
	  accOut[iSmpl] = acc;
	}
}

/**
 * Every symbol is a 10bit value in 2's complement format. However,
 * they are placed into a 16bit unsigned int with no sign extension.
 * This function sign extends the symbol values and then compare
 * them in order to determine if they correspond to a 1 or a 0 bit.
 * The following convention is assumed:
 *
 * if symb0 < symb1 => (-1) to (1) transition => bit 0
 * if symb0 >= symb1 =>  (1) to (-1) transition => bit 1
 *
 */
unsigned int decodeBit(unsigned int symb0, unsigned int symb1)
{
	int stmp0 = 0, stmp1 = 0;

	// test the 10th bit and sign extends to 16 bit signed int (sign extend)
	stmp0 = symb0 & 0x0200 ? (0xFC00 | symb0) : (0x03FF & symb0);
	stmp1 = symb1 & 0x0200 ? (0xFC00 | symb1) : (0x03FF & symb1);

	if (stmp0 > stmp1) { // bit 1
		return 1;
	} else {
		return 0;
	}
}

/**
 * Returns the correspondent data portion size in bits
 * for a given message length code.
 * OBS: Assuming a 28bit ID_NUMBER format
 */
unsigned int calcMessageLength(unsigned int msgByteLength)
{

	switch (msgByteLength) {
	case 0:
		return 56 /*4MSG Length + 28 bits ID + 24 bits MSG */;
		break;
	case 3:
		return 90 /*4+28+56 bits*/;
		break;
	case 5:
		return 120 /*4+28+88 bits*/;
		break;
	case 6:
		return 152 /*4+28+120 bits*/;
		break;
	case 9:
		return 184 /*4+28+152 bits*/;
		break;
	case 10:
		return 216 /*4+28+184 bits*/;
		break;
	case 12:
		return 248 /*4+28+216 bits*/;
		break;
	case 15:
		return 280 /*4+28+248 bits*/;
		break;
	default:
		return 56 /*4+28+248 bits*/;
		break;
	}
}

#define MAX_SYMBOLS_PER_WINDOW 8
#define MAX_SYNCH_SYMBOLS 128
#define SYNCH_MIN_MASK 0x3FF
#define SYNCH_PATTERN	0xFFFE2F
void frameSynch(PTTPackage_Typedef * wpckg, int pttd_symbol)
{
	if (wpckg->total_symbol_cnt > MAX_SYNCH_SYMBOLS) { // synch failed
		wpckg->status = PTT_ERROR;
		printf("FS FAIL\n\n");
	} else {
		wpckg->symb_array[0] = wpckg->symb_array[1]; // previous
		wpckg->symb_array[1] = pttd_symbol; // current
		
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
		//		printf("SYNC OK!!!!\n\n");
				wpckg->status = PTT_DATA;
				wpckg->symb_cnt = 0;
			}
		}
	}
}

#define MAX_SYMBOLS_PER_WINDOW 8
void readData(PTTPackage_Typedef * wpckg, int pttd_symbol) 
{
	unsigned int i0;
	unsigned int msgBitLength;
	unsigned int tmp0;
	int userData;

	if (wpckg->symb_cnt < MAX_SYMBOLS_PER_WINDOW) {
		// just accumulate 8 symbols for now
		wpckg->symb_array[wpckg->symb_cnt++] = pttd_symbol;
	}
	// all message length symbols were received
	if (wpckg->symb_cnt == MAX_SYMBOLS_PER_WINDOW) {
		for (i0 = 0; i0 < MAX_SYMBOLS_PER_WINDOW; i0 += 2) {
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

int VgaGain (int tmp_amp)
{
	int tmp_vga_mant, tmp_vga, tmp_vga_exp;
	double log2_vgaGain;
	double log (double);
	double pow (double, double);
	double roof;
	double log_amp, amp_in;
	double mantcalc;
	double exp_mantcalc;
	const double m_LOG2E = 1.442695; //1/log2(e)
	roof = log(256);
	amp_in = tmp_amp*160; //gain of matched filter(160) and cordic(1.6) mult_cplx
	log_amp = log(amp_in);
	if(amp_in>256){
		log2_vgaGain = (log_amp-roof) * m_LOG2E;
		tmp_vga_exp = (int)log2_vgaGain;
	}else{
		log2_vgaGain = (roof-log_amp) * m_LOG2E;
		tmp_vga_exp = (int) ceil(log2_vgaGain);
	}
	tmp_vga_exp = tmp_vga_exp & 0x3F;
	if(tmp_vga_exp>log2_vgaGain){
		exp_mantcalc = (double)tmp_vga_exp + log2_vgaGain;
	}else{
		exp_mantcalc = (double)tmp_vga_exp - log2_vgaGain;
	}
	mantcalc = pow(2,exp_mantcalc);
	tmp_vga_mant = (floor(mantcalc*256));
	tmp_vga_mant = tmp_vga_mant & 0xFF;
	tmp_vga_mant = tmp_vga_mant << 6;
	tmp_vga = tmp_vga_exp + tmp_vga_mant;
	return tmp_vga;
}

/*************************
*/
/* Factored discrete Fourier transform, or FFT, and its inverse iFFT */
#ifndef PI
#define PI	3.14159265358979323846264338327950288
#endif

/*
   fft(v,N):
   [0] If N==1 then return.
   [1] For k = 0 to N/2-1, let ve[k] = v[2*k]
   [2] Compute fft(ve, N/2);
   [3] For k = 0 to N/2-1, let vo[k] = v[2*k+1]
   [4] Compute fft(vo, N/2);
   [5] For m = 0 to N/2-1, do [6] through [9]
   [6]   Let w.re = cos(2*PI*m/N)
   [7]   Let w.im = -sin(2*PI*m/N)
   [8]   Let v[m] = ve[m] + w*vo[m]
   [9]   Let v[m+N/2] = ve[m] - w*vo[m]
 */



int fft(float complex *tmp, float complex *v, int n)
{
	if(n>1) {	// otherwise, do nothing and return 
	int k,m;   
	float complex z, w;
        float complex * vo;
        float complex * ve;

	ve = tmp; vo = tmp+n/2;				
	for(k=0; k<n/2; k++) {
		ve[k] = v[2*k];
		vo[k] = v[2*k+1];
	}
	fft(v, ve, n/2);// FFT on even-indexed elements of v[]
	fft(v, vo, n/2);// FFT on odd-indexed elements of v[] 
	for(m=0; m<n/2; m++) {
		w = cos(2*PI*m/(double)n)-(sin(2*PI*m/(double)n)*I);   
		z = w*vo[m];      
		v[m] = ve[m] + z;      
		v[m+n/2] = ve[m]-z;  
	
	
	}
	}

  return 0;
}

void bitInvert(float complex * a, int N)
{
	int i,j,k,mv,rev;
	float complex aux;
	for(i=1; i<N;i++){
		k=i;
		mv = N/2;
		rev = 0;
		while(k>0){
			if((k%2)>0)
				rev=rev+mv;
			k = k/2;
			mv=mv/2;
		}
		{
		if(i<rev){
			aux=a[rev];
			a[rev]=a[i];
			a[i]=aux;
		}
		}
	}

}
void fft_step(float complex * a,int N)
{
	int i,k,m;
	float complex w,v,h;
	k=1;
	while(k<=N/2){
		m=0;
		while(m<=(N-2*k)){
			for(i=m;i<m+k;i++){
				w=cos(PI*(double)(i-m)/(double)(k))-(sin(PI*(double)(i-m)/(double)(k))*I);
				h=w*a[i+k];
				v=a[i];
				a[i]=h+a[i];
				a[i+k]=v-h;
			}
			m=m+2*k;
		}
		k=k*2;
	}
}
void fft_it(float complex * a, int N)
{
	bitInvert(a,N);
	fft_step(a,N);
}
