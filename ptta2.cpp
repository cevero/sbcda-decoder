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
        const int fs = 128*1000;
        const int bitrate = 400;
        const int angmode = M_PI/3;
        float tcarrier = 0.16;
        const int syncpattern[24] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0,1,1,1,1};
        int usermsglength;
        int timelength;

        //methods
        //
        ptta2(int msglentype);        

        inline ~ptta2()
        {
            delete this->psf;
        }

        inline int getmsg()
        {
            return usermsg;
        }

        inline int get_usermsglength(){
            return 24+32*msglentype;
        }

        int get_timelength();

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
    for(int i=0;i<plusbetasmpl*2+1;i++)this->psf[i]=0;
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

int ptta2::get_timelength()
{
    int pskrate = 2*this->bitrate;
    float beta = 0.125;
    float a = ((1+beta)/2*(this->fs/pskrate));
    int value = 24+get_usermsglength()/this->bitrate+this->tcarrier+(a+(0.5>=a))/this->fs;
    return value;
}

//
int main()
{
    ptta2 teste(100);
    cout<<"O Tamanho da mensagem eh: "<<teste.get_timelength()<<endl;
    return 0;
}
