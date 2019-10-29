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
        inline ~ptta2()
        {
            delete this->psf;
        }
        inline int getmsg()
        {
            return usermsg;
        }
        const int fs = 128*1000;
        const int bitrate = 400;
        const int angmode = M_PI/3;
        float tCarrier = 0.16;
        const int syncpattern[24] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0,1,1,1,1};
        int usermsglength;
        int timelenght;

        //methods
        //
        ptta2(int msglentype);        
};

ptta2::ptta2(int msglentype)
{
    if (msglentype==0)cout<<"Error: msgLenType invalid\n";
    else this->msglentype = msglentype;
    //generate randonmsg makes non sense
    //
    float beta = 0.125; //value between 0 and 1
    int pskrate = 2*this->bitrate;
    float auxlessbetasmpl = (1-beta)/2*(this->fs/pskrate);
    int lessbetasmpl = auxlessbetasmpl;
    lessbetasmpl+=lessbetasmpl+(0.5>=(auxlessbetasmpl/lessbetasmpl));
    float auxplusbetasmpl = (1+beta)/2*(this->fs/pskrate);
    int plusbetasmpl = auxplusbetasmpl;
    plusbetasmpl+=plusbetasmpl+(0.5>=(auxplusbetasmpl/plusbetasmpl));
    
    this->psf = new int[plusbetasmpl*2+1];
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


int main()

{
    ptta2 teste(100);
    cout<<"O Tamanho da mensagem eh: "<<teste.fs<<endl;
    return 0;


}
