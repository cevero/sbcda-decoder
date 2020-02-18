#include "service.h"
// #include "cicFilterCplxStep.h"
// #include "sampler.h"
#include "pttA2Demod.h"

#define NUMBER_OF_SAMPLES 140800
#define NOUT	752
#define inputSeqW 1280

int main(int argc, char *argv[]){
  int i0,i1;
  FILE* inputFile = fopen("inputDemod.txt","r");

  if(inputFile == NULL){
    printf("Can't open file!\n");
    return 1;
  }
  char input[60];
  int inputRe[NUMBER_OF_SAMPLES];
  int inputIm[NUMBER_OF_SAMPLES];

  for (i0=0;fgets(input,sizeof(input), inputFile) != NULL;i0++){
    sscanf(input,"%d %d",&inputRe[i0], &inputIm[i0]);
    //printf("%02d: %06d %06d\n", i0, inputRe[i0], inputIm[i0]);
  }
	 //printf("%02d: %06d %06d\n", 0, inputRe[1], inputIm[1]);
   fclose(inputFile);

	 inputFile = fopen("inputParameters.txt","r");

  if(inputFile == NULL){
    printf("Can't open parameters file!\n");
    return 1;
  }
   
  int ncoInitFreq, vgaMant, vgaExp;
	for (i0=0;fgets(input,sizeof(input), inputFile) != NULL;i0++){
    sscanf(input,"%d %d %d",&ncoInitFreq, &vgaMant, &vgaExp);
    //printf("%02d: %06d %06d\n", ncoInitFreq, vgaMant, vgaExp);
	}
  fclose(inputFile);

	FILE* outputFile = fopen("outputDemod.txt","r");
    
  if(outputFile == NULL){
    printf("Can't open file!\n");   
    return 1;
  }
  char output[20];
  int RefsymbLock[NOUT];
  int Refsymb[NOUT];
	int ch[NOUT];
	int nErr = 0;

  for (i0=0;fgets(output,sizeof(output), outputFile) != NULL;i0++){
    sscanf(output,"%d %d %d",&ch[i0], &RefsymbLock[i0],&Refsymb[i0]);
    //printf("%d: [%d]  %d\n", i0, RefsymbLock[i0],Refsymb[i0]);
  }
  fclose(outputFile); 

  printf("-------> Finish Load Input Signal <-------\n\n");		
  //printf("%d\n", inputIm[639]);
   
  /********************************************************************************/
  int previousAccRe;
  int previousAccIm;
  int accRe, accIm, accDlyIdx;


  int complex inputSignal[] = {[0 ... inputSeqW-1] = 0+0*I};
  float complex  demodSignal[] = {[0 ... smplPerSymb/deciRate-1] = 0};
  int symbOutBuffer[2][NUMBER_OF_SAMPLES/smplPerSymb];

	sampler_mem * str_smp = malloc(sizeof(sampler_mem));
 	str_smp->smplBuffer = malloc(2*smplPerSymb*(sizeof(int)));
	str_smp->lutSign = 1;
  str_smp->symbLock = 0;
	str_smp->delayAcc = 0;
  str_smp->prevDelay = 0;

  mem_cic * str = malloc(sizeof(mem_cic));
	str->previousAccRe = malloc(delayIdx*sizeof(int));
	str->previousAccIm = malloc(delayIdx*sizeof(int));
	str->accRe = 0;
  str->accIm = 0;    
  str->accDlyIdx = delayIdx-1;		
  for (i0 = 0; i0<delayIdx;i0++){
    str->previousAccRe[i0]=0;
    str->previousAccIm[i0]=0;
  }

	mem_cic * str1 = malloc(sizeof(mem_cic));
	str1->previousAccRe = malloc(delaySmp*sizeof(int));
	str1->previousAccIm = malloc(delaySmp*sizeof(int));
	str1->accRe = 0;
  str1->accIm = 0;    
  str1->accDlyIdx = delaySmp-1;		
  for (i0 = 0; i0<delaySmp;i0++){
    str1->previousAccRe[i0]=0;
    str1->previousAccIm[i0]=0;
  }

	demod_mem * p = malloc(sizeof(demod_mem));
	p->symbLock = malloc(nSymb*sizeof(int));
	p->symbOut = malloc(nSymb*sizeof(int));
	for (i0 = 0; i0<nSymb;i0++){
    p->symbLock[i0] = 0;
		p->symbOut[i0] = 0;
  }
	p->ncoDFreq = 0;
	p->symbCount = 0;
	p->lfAcc = 0;
	p->thetaNco = 0;
   
   
   int complex OutBuffer[] = {[0 ... NOUT] = 0};		
   printf("----------------> Test <------------------\n\n");
		
  for (i0=0;i0< NUMBER_OF_SAMPLES/(smplPerSymb*nSymb) ;i0++){
    for(i1=0;i1<inputSeqW;i1++){
      inputSignal[i1] = inputRe[smplPerSymb*nSymb*i0+i1]+inputIm[smplPerSymb*nSymb*i0+i1]*I;
			//printf("%f %f\n", creal(inputSignal[i1]),cimag(inputSignal[i1]));
    }
		//printf("%f %f\n", creal(inputSignal[1279]),cimag(inputSignal[1279]));
    //printf("***---------- processing --------------**** [%d]\n", i0);
		//break;
		pttA2Demod(inputSignal, ncoInitFreq, vgaMant, vgaExp, p, str, str1, str_smp);
		//printf("***---------- processing --------------**** [%d]\n", i0);
    for(i1=0;i1<nSymb;i1++){
    	//printf("%d %d\n", p->symbLock[i1],p->symbOut[i1]);
			symbOutBuffer[0][i1+i0*nSymb] = p->symbOut[i1];
			symbOutBuffer[1][i1+i0*nSymb] = p->symbLock[i1];
    }
  }

/* 	printf("***---------- results --------------**** [%d]\n", i0);
	for(i1=0;i1<90;i1++){
    printf("%d: %d %d\n",i1,symbOutBuffer[1][i1], symbOutBuffer[0][i1]);
  }
 */
	free(str_smp);
	free(str_smp->smplBuffer);	
	free(str1);
	free(str1->previousAccRe);
	free(str1->previousAccIm);
	free(str);				
	free(str->previousAccRe);
	free(str->previousAccIm);		
	free(p);
	free(p->symbLock);
	free(p->symbOut);
  printf("---------------> Checker <----------------\n\n");
   

//  Compare output file with outputSignal from code		
	for(i0 = 0;i0<NOUT;i0++ ){

		if((abs(symbOutBuffer[0][i0]-Refsymb[i0])>3)||(symbOutBuffer[1][i0]!=RefsymbLock[i0])){
		nErr++;
		printf("~~~~~ Error %d ~~~~~\n",i0);
		printf("Got: [%d] %d \n",symbOutBuffer[1][i0],symbOutBuffer[0][i0]);
		printf("Expected: [%d] %d \n",RefsymbLock[i0],Refsymb[i0]);				
		}		
	}
	if(nErr != 0){
		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		printf("~~~~~~~~ Try Again! %d mismatches ~~~~~~~~\n",nErr);
		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	}else{
		printf("***-----------------------------------****\n");
		printf("***----------- SUCCESS !! ------------****\n");
		printf("***-----------------------------------****\n");
	}
	
  return 0;
}