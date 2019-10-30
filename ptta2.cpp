#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <time.h>
using namespace std;

class Ptta2 {
  // Objects from this class contais all parameters of a PTT-A2 baseband signal.
    private:
        int usermsg;
        // userMsgLength information coded in a integer between 1 and 8.
        // msgLenType is used to select the User Message Length between the
        // following allowed values
        // 56, 88, 120, 152, 184, 216, 248 and 280.
        int msglentype;
        int* psf; // pulse shapping filter inpulse response
        
    public:
        const int fs = 128*1000;    // sample rate
        const int bitrate = 400;    // data rate in bits per second
        const int angmode = M_PI/3; // modution angle in rad
        float tcarrier = 0.16;      // pure carrier period time length
        // sync bit pattern
        const int syncpattern[24] = {1,1,1,1,1,1,1,1,1,1,1,1,
                                     1,1,1,0,0,0,1,0,1,1,1,1};
        int usermsglength;  // User Mensage Length in bits
        int timelength;     // signal time length in seconds

        //methods
        //
        Ptta2(int msglentype);        

        inline ~Ptta2()
        {
            delete this->psf;
        }

        inline int getmsg()
        {
            return usermsg;
        }
        inline int gettypemsg()
        {
        }
        inline int get_usermsglength(){
            return 24+32*msglentype;
        }

        int get_timelength();

};

Ptta2::Ptta2(int msglentype)
{
    if (msglentype==0)cout<<"Error: msgLenType invalid\n";
    else this->msglentype = msglentype;

    // Initalize User Message randomly
    //obj = obj.genRandUserMsg;


    // Generate trapezoidal pulse shapping filter
    // 'beta' is the transition period in multiple of PSK symbol period
    float beta = 0.125; //value between 0 and 1

    int pskrate = 2*this->bitrate;
    int lessbetasmpl = round((1-beta)/2*(this->fs/pskrate));
    int plusbetasmpl = round((1+beta)/2*(this->fs/pskrate));
    
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

int Ptta2::get_timelength()
{
    int pskrate = 2*this->bitrate;
    float beta = 0.125;
    float a = ((1+beta)/2*(this->fs/pskrate));
    int value = 24+get_usermsglength()/this->bitrate+this->tcarrier+
                                                (a+(0.5>=a))/this->fs;
    return value;
}

int main()
{
    srand(time(NULL));

    Ptta2 teste(rand()%8+1);

    for (int i = 0; i < 24; i++) {
        cout<<" "<<teste.syncpattern[i];
    }
    
    cout<<"\nprinta essa merda: "<<teste.get_usermsglength()<<endl;
    return 0;
}
