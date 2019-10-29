#include <iostream>
#include <math.h>
#include <stdlib.h>
using namespace std;

class ptta2 {
    //properties

    private:
        int usermsg;
        int msglentype;
        int* psf;
    public:
        inline int getmsg() return usermsg;
        const int fs = 128*1000;
        const int bitrate = 400;
        const int angmode = M_PI/3;
        tCarrier = 0.16;
        const int syncpattern[24] = [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0,1,1,1,1];
        int usermsglength;
        int timelenght;

        //methods
        //
        ptta2(int msglentype);        
}

ptta2(int msglentype)
{
    if (msglentype==0)cout<<"Error: msgLenType invalid\n";
    else this.msglentype = msglentype;
    //generate randonmsg makes non sense
    //
    float beta = 0.125; //value between 0 and 1
    int pskrate = 2*this.bitrate;
    float auxlessbetasmpl = (1-beta)/2*(this.fs/pskrate);
    int lessbetasampl = auxlessbetasmpl;
    lessbetasampl+=lessbetasampl+(0.5>=(auxlessbetasmpl/lessbetasampl));
    float auxplusbetasmpl = (1+beta)/2*(this.fs/pskrate);
    int plusbetasampl = auxplusbetasmpl;
    plusbetasampl+=plusbetasampl+(0.5>=(auxplusbetasmpl/plusbetasampl));
    
    this.psf = new int[plusbetasampl*2+1];
    int idx = 0;
    for(int n=-1*plusbetasampl;n<plusbetasampl;n++)
    {
        if(abs(n)<=lessbetasampl)
            this.psf[idx]=1;
        else
            this.psf[idx]=1-(abs(n)-lessBetaSmpl)/(plusBetaSmpl-lessBetaSmpl);
    idx = idx+1;
}
