#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ptta2.h"

class SimParam
{
    public:
        string fileName;      // name of the file that will store the simulation result
        string comment;       // a sentece describing the simulation scenario
        bool reset;           // if reset=1 overwrite previous saved simulation
        float n0DbFs;         // noise power density in dB ratio to ADC Clip 
                              // (Saturation) Point Power
        float fullScaleAmp;   // ADC full scale amplitude
        list<Ptta2> pttList;  // cell list containing PttA2 and PttA3Nz objects
        float maxPwrDbN0;     // Maximum PTT power (realistic estimation)
        float dynRangeDb;     // dB ratio between strongest and weakest PTT signal. 
        list<float> peakPwrDbN0;    // Worst case instantaneous power peak in dB ratio to N0.
        float pwrDbN0List;    // PTTs power in dB ratio to No (noise power density)
        float thetaDegList;   // PTTs initial phase in degree
        float freqHzList;     // PTTs initial frequency in Hz
        float hzPerSecList;   // PTTs Doppler Rate in Hz/s
        float tSim;           // simulation time length in second
        float timeList;       // PTTs initial time in seconds
        bool vectSignalGen;   // Generate signal for Vect. Signal Generator

        int nDecoder=12;      // Number of PTT Decoder channel
        int freqW=20;         // Decoder frequency word width

    private:
        list<int> typeList;        // PTT signals type (A2-> 2, A3/Nz->3)
        list<int> userMsgLenList;  // List of User Message Lengths in bits

    protect:
        int nPtt;            // number of PTT signal in simulation
        float fs = 128e3;      // sample frequency
//methods


        int* get_typelist();
        int* get_usermsglenlist();
        SimParam(int nPtt,float tSim,list<int> typeList, list<int> usrMsgLenCode);
};
int* SimParam::get_typelist()
{
    list<int>value;
    for(int i=0;i<this.nPtt;i++){
        //por enquanto consideremos que temos somente ptta2
        value.pushback(2);
    }
}
int* SimParam::get_usermsglenlist()
{
    list<int>value;
    for(int i=0;i<this.nPtt;i++){
        value.push_back(this.pttList[i].userMsgLength);
    }

}

SimParam::SimParam(nPtt, tSim, typeList, usrMsgLenCode)
{
    
      // Create a SimParam object
      // SINTAX
      // obj = SimParam(nPtt, simTime, typeList, usrMsgLenCode)
      // nPtt: number of Ptt signals. If undefined, it assumes 10.
      // tSim: simulation time length in seconds.
      // typeList: a 1 x nPtt array that specifies each Ptt signal type as PTT-A2 
      // or PTT-A3 using the values of 2 and 3, respectively.
      // usrMsgLengCode: 1 x nPtt array that specifies each Ptt signal length
      // with an interger value from 0 to 8 for PTT-A3, and from 1 to 8 for
      // PTT-A2.
      // If tSim is not provided it will receive the default value of nPtt/5+1;
      // If typeList and usrMsgLenCode are not provided they will be randomized.       

      // set a default value for input arguments, if not provided. 

    nPtt=10;
    tSim=nPtt/5+1;
    for(int i=0;i<nPtt;i++){
        typeList.push_back(2);
        usrMsgLenCode.push_back(rand()%9+1);
        pttList(i) = PttA2(usrMsgLenCode);
    }
    this.nPtt=nPtt;
    this.reset=0;
    
//cell line 103 simparam
    
    //Set Simulation time
    this.tSim=tSim;

    // Random carrier power, phase and doppler
    this.maxPwrDbN0 = 64;
    this.dynRangeDb = 24;
    for(int i=0;i<nPtt;i++){
        this.pwrDbN0List.push_back(this.maxPwrDbN0-this.dynRangeDb*(rand()%9)/9);
        this.thetaDegList.push_back(360.0*rand()%1000/1000);
        this.hzPerSecList.push_back(240.0*rand()%1000/1000-120);
        this.timeList.push_back((rand()%1000)/1000*(tSim-1));
        this.freqHzList.push_back(rand()%1000/1000*60e3-30e3);
        }

    timeList.sort();
   
    this.fullScaleAmp = pow(2,15)-1;
    this.n0DbFs = -118.5; //-118.5 //is the value measured
    this.peakPwrDbN0 = obj.maxPwrDbN0+20; // 86 dBm - (-174 dBm/Hz)
    this.vectSignalGen = false;

}


