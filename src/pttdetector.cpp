#include <stdio.h>
#include <stdlib.h>
#include <math.h>

class pttdetector{

    //pttDetector
    //Receive as input a sequence of 2048 input samples
    //Properties constants

        const int   windowLength  = 1280;   // Window length in samples
        const int   dftLength     = 2048;
        const int   dftFreqW      = 11;     // log2(dftLength)
        const int   sampleRate    = 128e3;  // Sample rate in Hz
        const int   bw0Hz         = 1600;   // signal bandwidth in Hz
        const int   bw1Hz         = 2400;   // signal bandwidth in Hz
        const int   bw2Hz         = 3200;   // signal bandwidth in Hz
        const int   worstCNO      = 40;     // Minimum detectable Carry to N0 dB ratio (aprox 12.7 Eb/N0)  

        int  nDecoder;     // Number of decoders
        int  window;       // DFT window
        int  freqW;        // Frequency resolution in number of bits
        int  threshold;    // Detection threshold in Amplitude
        int  bw0;          // Signal bandwidth in multiple of sampleRate/dftLength
        int  bw1;          // Signal bandwidth upto 5th harmonic
        int  bw2;          // Signal bandwidth upto 7th harmonic
        int  prevPass;
        pttdetector(param);

};

pttdetector(param)
