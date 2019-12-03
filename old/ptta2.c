#include <math.h>
#include "ptta2.h"

Ptta2::Ptta2(int msglentype)
{
    if (msglentype==0)
        //cout<<"Error: msgLenType invalid\n";
        throw(1);
    else
        this->msglentype = msglentype;

    // Initalize User Message randomly
    //obj = obj.genRandUserMsg;


    // Generate trapezoidal pulse shapping filter
    // 'beta' is the transition period in multiple of PSK symbol period
    float beta = 0.125; //value between 0 and 1

    int pskrate = 2*this->bitrate;
    int lessbetasmpl = round((1-beta)/2*(this->fs/pskrate));
    int plusbetasmpl = round((1+beta)/2*(this->fs/pskrate));
    
    this->psf = new int[plusbetasmpl*2+1];
    for (int i=0;i<plusbetasmpl*2+1;i++)
        this->psf[i]=0;
    int idx = 0;

    for(int n=-1*plusbetasmpl;n<plusbetasmpl;n++)
    {
        if(abs(n)<=lessbetasmpl)
            this->psf[idx]=1;
        else
            this->psf[idx]=1-(abs(n)-lessbetasmpl)/(plusbetasmpl-lessbetasmpl);
        idx = idx+1;
    }
}

int Ptta2::get_timelength()
{
    int pskrate = 2*this->bitrate;
    float beta = 0.125;
    float a = ((1+beta)/2*(this->fs/pskrate));
    int value = 24+get_usermsglength()/this->bitrate+this->tcarrier+
                                                (a+(0.5>=a))/this->fs;
    return value;
}
/*
    function value = digitalMsg(obj)
    % Return the Ptt-A2 Digital Message.
    % Overall bits entering the modulator, as defined in the spec.
      value = [obj.synchPattern obj.userMsg];
    end
    
    function obj = genRandUserMsg(obj)
      % Randomize User Message (userMsg)

      % convert mLenCodeList in a bit vector
      aux = dec2bin(obj.msgLenType-1, 3) == '1';
      
      % generate the parity bit
      parity = mod(sum(aux), 2);
      
      % append the parity bit to the bit vector
      mLenCodeBitVec = [aux parity];
      
      % generate random ID number and User Data
      randomDataBitVec = round(rand(1,obj.userMsgLength-4));
      
      % full user message generation
      obj.userMsg = [mLenCodeBitVec randomDataBitVec];
    end
    
    
    function pttSignal = signalGen(obj)
      % Generate the PTT Signal
      
      % Append synchPattern to userMsg to create the full tx bit vector
      bitVec =  [obj.synchPattern obj.userMsg];
      
      % apply biphase_L coding 1-->[+1 -1], 0-->[-1 +1];
      symbVec0 = 2*bitVec-1;
      symbVec1 = [symbVec0 ; -symbVec0];
      symblVec2 = reshape(symbVec1, 1, []);
      
      % add zeros for Pure Carrier period
      symblVec = [zeros(1, obj.tCarrier*2*obj.bitRate) symblVec2];
      
      % constelation mapping
      symbVec = exp(1j*obj.angMod*symblVec);
      
      % upsampling according to ratio between sample frequency and 2*BitRate.
      upSmplRate = obj.fs/(2*obj.bitRate);
      pttSignal = upsample(symbVec, upSmplRate);
      
      % apply pulse shapping filter
      % rectangular shape filter is used
      % rec = ones(1,upSmplRate);
      % pttSignal = filter(rec, 1, pttSignal);
      
      % trapezoidal shape filter
      pttSignal = conv(obj.psf, pttSignal);
    end
*/
