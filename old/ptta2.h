#ifndef PTTA2_H
#define PTTA2_H

typedef struct ptta2_t{
    float *usermsg;
    int msglentype;
    float *psf;
    int fs;         // sample rate
    int bitrate;    // data rate in bits per second
    float angmode;  // modution angle in rad (pi/3)
    float tcarrier; // pure carrier period time length
    
    int syncpattern[24];// sync bit pattern
    int usermsglength;  // User Mensage Length in bits
    float timelength;     // signal time length in seconds
} ptta2_t;

#endif
