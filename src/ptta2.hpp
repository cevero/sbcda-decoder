#ifndef PTTA2_HPP
#define PTTA2_HPP

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

#endif
