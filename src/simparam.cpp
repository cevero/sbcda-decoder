#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <array>
#include <list>
#include <string>
#include "ptta2.hpp"
using namespace std;

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
        float peakPwrDbN0;    // Worst case instantaneous power peak in dB ratio to N0.
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

};
