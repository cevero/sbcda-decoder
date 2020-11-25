#ifndef SERVICE_H
#define SERVICE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include "rt/rt_api.h"
#include <complex.h>

typedef enum{
	PTT_FREE=0, /**<  Package clean */
	PTT_FRAME_SYNCH, /**<  Package is in Frame Synch phase. */
	PTT_DATA,/**<  Package is receiving the PTT Sensor data bits */
	PTT_ERROR, /**<  Package failed the Fram Synch phase. */
	PTT_READY /**<  Package is is ready to be transmited to the on-board computer. */
}PTTDecode_Status;

typedef struct {
int frameType; /**< Identification byte of the PTT Package. */
int timeTag;/**< Time tag indicating when the package was decoded */
int errorCode;/* 0: no error; 1: parity error  */
int carrierFreq;/* Carrier Frequency (kHz=carrierFreq*128/2^20+401635)*/
int carrierAbs; /* Carrier Amplitude at ADC interface output */
int msgByteLength; /**< Message Length parameter */
int userMsg[35];
//***************************************************************
//   THIS FILEDS ARE FOR INTERNAL CONTROL ONLY,			*
//   THEY ARE NOT SENT TO ON-BOARD COMPUTER AS PART		*
//   OF THE PACKAGE						*
//   								*
int synch_patternA;//						*
int synch_patternB;//						*
int status; //< Current decoding state of the PTT Package 	*
int symb_array[8];//						*
int total_symbol_cnt;//						*
int bit_cnt; //< Number of bits of the PTT message 		*
int symb_cnt;//							*
//***************************************************************

} PTTPackage_Typedef;

void incAndOvFlow(int prevAcc,int inc,int nInc,int accWidth, int * accOut);
unsigned int decodeBit(unsigned int symb0, unsigned int symb1);
unsigned int calcMessageLength(unsigned int msgByteLength);

void frameSynch(PTTPackage_Typedef * wpckg, int pttd_symbol);

void readData(PTTPackage_Typedef * wpckg, int pttd_symbol);

int VgaGain (int tmp_amp);

int fft(float complex *tmp, float complex *v, int n);
//int fft(int complex *v, int n);
//int _fft(int complex * buf, int complex *out, int n, int step);

void fft_it(float complex * a, int N);
void fft_step(float complex * a,int N);
void bitInvert(float complex * a,int N);


typedef struct{
  float r;
  float i;
}cpx;

void fft_itO(cpx * a,cpx * we,int N);
void fft_stepO(cpx * a,cpx * we,int N);
void bitInvertO(cpx * a,int N);
cpx KSUM(cpx a, cpx b);
cpx KDIFF(cpx a, cpx b);
cpx KPROD(cpx a, cpx b);

#endif
