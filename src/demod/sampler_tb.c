#include "service.h"
#include "sampler.h"
//#include "cicFilterCplxStep.h"
#define inputSeqW 8
#define smplPerSymb 8
#define delayIdx 32
#define NUMBER_OF_SAMPLES 2432
#define NOUT	NUMBER_OF_SAMPLES/smplPerSymb
int main(int argc, char *argv[]){
   int i0,i1;
   FILE* inputFile = fopen("inputSampler.txt","r");
   if(inputFile == NULL){
       printf("Can't open file!\n");
       return 1;
   }
   char input[10];
   int inputIm[NUMBER_OF_SAMPLES];
   for (i0=0;fgets(input,sizeof(input), inputFile) != NULL;i0++){
       sscanf(input,"%d", &inputIm[i0]);
   }
   fclose(inputFile);
   printf("-------> Finish Load Input Signal <-------\n\n");		
   //printf("%d\n", inputIm[639]);
   /********************************************************************************/
   int demodSignal[] = {[0 ... smplPerSymb-1] = 0};
  mem_cic * str = malloc(sizeof(mem_cic));
	str->previousAccRe = malloc(delayIdx*sizeof(int));
	str->previousAccIm = malloc(delayIdx*sizeof(int));
	sampler_mem * str_smp = malloc(sizeof(sampler_mem));
	str_smp->smplBuffer = malloc(2*smplPerSymb*(sizeof(int)));
	str->accRe = 0;
  str->accIm = 0;    
  str->accDlyIdx = delayIdx-1;		
  str_smp->lutSign = 1;
  str_smp->symbLock = 0;
  for (i0 = 0; i0<delayIdx;i0++){
    str->previousAccRe[i0]=0;
    str->previousAccIm[i0]=0;
  }
  str_smp->delayAcc = 0;
  str_smp->prevDelay = 0;
  int smplBuffer [] = {[0 ... 2*smplPerSymb-1] = 0};
	int symbOutBuffer[2][NUMBER_OF_SAMPLES/smplPerSymb];
  printf("----------------> Test <------------------\n\n");
  for (i0=0;i0<NUMBER_OF_SAMPLES/smplPerSymb;i0++){
    for(i1=0;i1<inputSeqW;i1++){
      demodSignal[i1] = inputIm[smplPerSymb*i0+i1];
    }
    //printf("***---------- processing --------------**** [%d]\n", i0);
		sampler(demodSignal, str_smp, str);
    symbOutBuffer[0][i0] = str_smp->symbOut;
		symbOutBuffer[1][i0] = str_smp->symbLock;
		//printf("[%d] %d\n", symbOutBuffer[1][i0],symbOutBuffer[0][i0]);
  }
  printf("---------------> Checker <----------------\n\n");
  FILE* outputFile = fopen("outputSampler.txt","r");
  if(outputFile == NULL){
      printf("Can't open file!\n");
      return 1;
  }
  char output[20];
  int symbLock[NOUT];
  int symb[NOUT];
	int nErr = 0;
  for (i0=0;fgets(output,sizeof(output), outputFile) != NULL;i0++){
      sscanf(output,"%d %d",&symb[i0], &symbLock[i0]);        
      //printf("%d: [%d]  %d\n", i0, symbLock[i0],symb[i0]);
  }
  fclose(outputFile);
  // Compare output file with outputSignal from code		
	for(i0 = 0;i0<NOUT;i0++ ){
			//printf("Got: [%d] %d \n",symbOutBuffer[1][i0],symbOutBuffer[0][i0]);
			//printf("Expected: [%d] %d \n",symbLock[i0],symb[i0]);	
		if((symbOutBuffer[0][i0]-symb[i0]>3)||(symbOutBuffer[1][i0]!=symbLock[i0])){
			nErr++;
			printf("~~~~~ Error %d ~~~~~\n",i0);
			printf("Got: [%d] %d \n",symbOutBuffer[1][i0],symbOutBuffer[0][i0]);
			printf("Expected: [%d] %d \n",symbLock[i0],symb[i0]);				
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
	free(str_smp);
  free(str->previousAccRe);
	free(str->previousAccIm);
  free(str);
  return 0;
}
