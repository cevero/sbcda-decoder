#include <iostream>
#include <math.h>

class pttdetctor{
       int const windowLength = 1280;   // Window length in samples
       int const ftLength     = 2048;
       int const ftFreqW      = 11;     // log2(dftLength)
       int const ampleRate    = 128e3;  // Sample rate in Hz
       int const w0Hz         = 1600;   // signal bandwidth in Hz
       int const w1Hz         = 2400;   // signal bandwidth in Hz
       int const w2Hz         = 3200;   // signal bandwidth in Hz
       int worstCNO           = 40;     // Minimum detectable Carry to N0 dB ratio (aprox 12.7 Eb/N0)
       int nDecoder;                    // Number of decoders
       int window;                      // DFT window
       int freqW;                       // Frequency resolution in number of bits
       float threshold;                 // % Detection threshold in Amplitude
       float bw0;                       // % Signal bandwidth in multiple of sampleRate/dftLength
       float bw1;                       // % Signal bandwidth upto 5th harmonic
       float bw2;                       // % Signal bandwidth upto 7th harmonic
       float prevPass;


}
