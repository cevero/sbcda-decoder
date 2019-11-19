#include "SimParam.hpp"
#include <math.h>
//%% STEP 1
//% Generate an input signal of 10 sec with 100 PTT signals with .6 second
//% each and without conflic between them.
//
//% create a SimParam object

int nPtt = 50;
int tSim = 10;
int maxPwrDbN0 = 64;
int dynRangeDb = 24;
int worstCNO = maxPwrDbN0 - dynRangeDb;
int typeList[nPtt];
int usrMsgLenCode[nPtt];

for (int i = 0; i < nPtt; i++) {
   typeList[i]=2;
   usrMsgLenCode[i]=8;
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


