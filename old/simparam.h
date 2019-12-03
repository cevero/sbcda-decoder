#ifndef SIMPARAM_H
#define SIMPARAM_H
#include "ptta2.h"

typedef struct simparam_t{
    float noDbFs;
    int fullScaleAmp;
    int maxPwrDbN0;
    int dynRangeDb;
    int peakPwrDbN0;
    int nPtt;
    int tSim;
    int vectSignalGen;
    int nDecoder;
    int freqW;
    int fs;
    
    float *pwrDbN0List;
    float *thetaDegList;
    float *freqHzList;
    float *hzPerSecList;
    float *timeList;
    int *typeList;
    int *userMsgLenList;

    ptta2_t *pttList;

} simparam_t;

#endif
