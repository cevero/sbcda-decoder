#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include "simparam.h"
#include "ptta2.h"

//%% STEP 1
//% Generate an input signal of 10 sec with 100 PTT signals with .6 second
//% each and without conflic between them.
//
//% create a SimParam object

simparam_t init_simparam(int nptt);
ptta2_t init_ptta2(int nptt);
float complex line_to_complex(char *str);

int main(int argc, const char *argv[])
{
    /*int worstCNO = maxPwrDbN0 - dynRangeDb;*/
    /*int typeList[nPtt];*/
    /*int usrMsgLenCode[nPtt];*/
    FILE *fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int complex z[1280];

    fp = fopen("../matlab/inputSeq.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    int iter = -1;
    char line_buffer[20];
    while ((read = getline(&line, &len, fp)) != -1) {
        if (iter == -1) {
            iter++;
            continue;
        }
        strcpy(line_buffer,line);
        z[iter] = line_to_complex(line_buffer);
        iter++;
        printf(line);
        printf("\n");
        printf("%.2f%+.2fi\n", creal(z[iter]), cimag(z[iter]));
    }

    fclose(fp);

    int nptt = 50;
    simparam_t simparam = init_simparam(nptt);
    printf("Thank you, but I'm not done yet!\n"); 
    printf("%d\n",simparam.fullScaleAmp);
    /*for (int j = 0; j < nptt; j++) {*/
        /*for (int i = 0; i < 24; i++) {*/
            /*printf("%d, ", simparam.pttList[j].syncpattern[i]);*/
        /*}*/
        /*printf("\n");*/
    /*}*/
    return 0;
}

float complex line_to_complex(char *str)
{
	char delim[] = " ";

    float real, imag;

	char *ptr = strtok(str, delim);

    real = atof(ptr);
    ptr = strtok(NULL, delim);
    imag = atof(ptr);

    return real+imag*I;
}

simparam_t init_simparam(int nptt)
{
    simparam_t simparam;

    simparam.noDbFs = -118.5;
    simparam.fullScaleAmp = 32767;
    simparam.maxPwrDbN0 = 64;
    simparam.dynRangeDb = 24;
    simparam.peakPwrDbN0 = 84;
    simparam.nPtt = nptt;
    simparam.tSim = 10;
    simparam.vectSignalGen = 0;
    simparam.nDecoder = 12;
    simparam.freqW = 20;
    simparam.fs = 128000;
    
    simparam.pwrDbN0List = malloc(sizeof(float)*nptt);
    simparam.thetaDegList = malloc(sizeof(float)*nptt);
    simparam.freqHzList = malloc(sizeof(float)*nptt);
    simparam.hzPerSecList = malloc(sizeof(float)*nptt);
    simparam.timeList = malloc(sizeof(float)*nptt);
    simparam.typeList = malloc(sizeof(int)*nptt);
    simparam.userMsgLenList = malloc(sizeof(int)*nptt);

    for (int i = 0; i < nptt; i++) {
       simparam.typeList[i]=2;
       simparam.userMsgLenList[i]=8;
    }

    simparam.pttList = malloc(sizeof(ptta2_t)*nptt);
    for (int i = 0; i < nptt; i++) {
        simparam.pttList[i] = init_ptta2(nptt);
    }

    return simparam;
}

ptta2_t init_ptta2(int nptt)
{
    ptta2_t ptta2;
    ptta2.msglentype = 8;
    ptta2.fs = 128*1000;    // sample rate
    ptta2.bitrate = 400;    // data rate in bits per second
    ptta2.angmode = 1.0472; // modution angle in rad (pi/3)
    ptta2.tcarrier = 0.16;      // pure carrier period time length
    // sync bit pattern
    int syncpattern[24] = {1,1,1,1,1,1,1,1,1,1,1,1,
                           1,1,1,0,0,0,1,0,1,1,1,1};
    memcpy(ptta2.syncpattern, syncpattern, sizeof(syncpattern));
    ptta2.usermsglength = 24+32*ptta2.msglentype; // User Message Length in bits
    ptta2.timelength;     // signal time length in seconds

    ptta2.usermsg = malloc(sizeof(float)*nptt);
    ptta2.psf = malloc(sizeof(float)*nptt);

    return ptta2;
}

