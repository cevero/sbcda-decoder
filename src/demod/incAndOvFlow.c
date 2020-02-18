#include "stdlib.h"
#include "math.h"
void incAndOvFlow(int prevAcc,int inc,int nInc,int accWidth, int * accOut)
{
	int acc = prevAcc;
	int upLimit = pow(2,accWidth);
	int iSmpl;

	//printf("0 acc: %d upLimit: %d\n",acc, upLimit);
	for (iSmpl=0;iSmpl<nInc;iSmpl++){
	  acc = acc + inc;
	  //printf("%d acc: %d upLimit: %d\n",iSmpl, acc, upLimit);
	  if (acc >= upLimit){
	    acc = acc-upLimit;
	  }else if (acc < 0){
	    acc = acc+upLimit;
	  }
	  accOut[iSmpl] = acc;
	}
}