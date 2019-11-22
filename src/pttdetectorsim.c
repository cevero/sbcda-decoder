#include "simparam.h"
#include "ptta2.h"
#include <math.h>

//%% STEP 1
//% Generate an input signal of 10 sec with 100 PTT signals with .6 second
//% each and without conflic between them.
//
//% create a SimParam object

int nptt = 50;
int worstCNO = maxPwrDbN0 - dynRangeDb;
int typeList[nPtt];
int usrMsgLenCode[nPtt];

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
       simparam.usrMsgLenCode[i]=8;
    }

    simparam.pttList = malloc(sizeof(ptta2_t)*nptt);
}

ptta2_t init_ptta2()
{
    ptta2_t ptta2;
    ptta2.msglentype = 8;
    ptta2.fs = 128*1000;    // sample rate
    ptta2.bitrate = 400;    // data rate in bits per second
    ptta2.angmode = 1.0472; // modution angle in rad (pi/3)
    ptta2.tcarrier = 0.16;      // pure carrier period time length
    // sync bit pattern
    ptta2.syncpattern[24] = {1,1,1,1,1,1,1,1,1,1,1,1,
                             1,1,1,0,0,0,1,0,1,1,1,1};
    ptta2.usermsglength = 24+32*ptta2.msgentype; // User Message Length in bits
    ptta2.timelength;     // signal time length in seconds

    ptta2.usermsg = malloc(sizeof(float)*nptt);
    ptta2.psf;
}

SimParam param(nPtt, tSim, typeList, usrMsgLenCode);
param = SimParam(nPtt, tSim, typeList, usrMsgLenCode);
param.maxPwrDbN0 = maxPwrDbN0;
param.pwrDbN0List = maxPwrDbN0-dynRangeDb*rand(1,param.nPtt);

for (int i = 0; i < nPtt; i++) {
    param.freqHzList.push_back((rand()%1000/1000)*60e3-30e3);
    param.hzPerSecList.push_back((rand()%1000/1000)*240-120);
    param.timeList.push_back((rand()%1000/1000)*(tSim-1));
}
param.timeList.sort();

//% Generate the input signal
float inputGain = 10^-((param.peakPwrDbN0+param.n0DbFs)/20);
PttA2 inputSignal = round(inputGain*signalGen(param));

//%% STEP 2
//% Break the input signal in windows of 1280 samples
//
//% compute number of windows
int nWindow = ceil(sizeof(inputSignal)/sizeof(PttA2)/1280);
//% adjust input signal length to a multiple of 1280 
nSmplToAdd = nWindow*1280 - length(inputSignal);
inputSignalExt = [inputSignal zeros(1, nSmplToAdd)];
//% create array of windows
inputWindArray = reshape(inputSignalExt, 1280, nWindow).';


