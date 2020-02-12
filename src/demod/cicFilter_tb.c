#include "service.h"
#include "cicFilterCplxStep.h"

#define NUMBER_OF_SAMPLES 640
#define NOUT	NUMBER_OF_SAMPLES/deciRate
int main(int argc, char *argv[]){
    int i0,i1;
    FILE* inputFile = fopen("inputCIC.txt","r");

    if(inputFile == NULL){
        printf("Can't open file!\n");
        return 1;
    }
    char input[30];
    int inputRe[NUMBER_OF_SAMPLES];
    int inputIm[NUMBER_OF_SAMPLES];

    for (i0=0;fgets(input,sizeof(input), inputFile) != NULL;i0++){
        sscanf(input,"%d %d",&inputRe[i0], &inputIm[i0]);
        //printf("%02d: %06d %06d\n", i0, inputRe[i0], inputIm[i0]);
    }
    fclose(inputFile);
    printf("-------> Finish Load Input Signal <-------\n\n");		
    //printf("%d\n", inputIm[639]);
    
    /********************************************************************************/
    int previousAccRe;
    int previousAccIm;
    int accRe, accIm, accDlyIdx;


    int complex cplxMultSignal[] = {[0 ... smplPerSymb*deciRate-1] = 0+0*I};
    float complex  demodSignal[] = {[0 ... length-1] = 0};
    float complex outputSignal[] = {[0 ... NUMBER_OF_SAMPLES/deciRate] = 0+0*I};

    mem * str = malloc(sizeof(mem));    
		str->accRe = 0;
    str->accIm = 0;    
    str->accDlyIdx = delayIdx-1;		
    for (i0 = 0; i0<delayIdx;i0++){
      str->previousAccRe[i0]=0;
      str->previousAccIm[i0]=0;
    }
    
    
    int complex OutBuffer[] = {[0 ... NOUT] = 0};		
    printf("----------------> Test <------------------\n\n");
		
    for (i0=0;i0<NUMBER_OF_SAMPLES/(smplPerSymb*deciRate);i0++){
        for(i1=0;i1<inputSeqW;i1++){
            cplxMultSignal[i1] = inputRe[smplPerSymb*deciRate*i0+i1]+inputIm[smplPerSymb*deciRate*i0+i1]*I;
						//printf("%f %f\n", creal(cplxMultSignal[i1]),cimag(cplxMultSignal[i1]));
        }
        //printf("***---------- processing --------------**** [%d]\n", i0);
        cicFilterCplxStep(cplxMultSignal, str, demodSignal);
        for(i1=0;i1<smplPerSymb;i1++){
        	//printf("%f %f*i\n", creal(demodSignal[i1]),cimag(demodSignal[i1]));
					outputSignal[i0*smplPerSymb+i1] = demodSignal[i1];
        }
    }
		
    printf("---------------> Checker <----------------\n\n");
    FILE* outputFile = fopen("outputCIC.txt","r");
    
    if(outputFile == NULL){
      printf("Can't open file!\n");
      return 1;
    }
    char output[30];
    float outRe[NOUT];
    float outIm[NOUT];
		int nErr = 0;

    for (i0=0;fgets(output,sizeof(output), outputFile) != NULL;i0++){
        sscanf(output,"%f %f",&outRe[i0], &outIm[i0]);
        //printf("%d: %f %f\n", i0, outRe[i0],outIm[i0]);
				
    }
    fclose(outputFile);
    // Compare output file with outputSignal from code		
		for(i0 = 0;i0<NOUT;i0++ ){
			if(((creal(outputSignal[i0]))-outRe[i0]>3) || ((int)(cimag(outputSignal[i0]))-outIm[i0])>3){                
				nErr++;
				printf("~~~~~ Error %d ~~~~~\n",i0);
				printf("Got: %f %f \n",creal(outputSignal[i0]),cimag(outputSignal[i0]));
				printf("Expected: %f %f \n",outRe[i0],outIm[i0]);				
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

		free(str);
    return 0;
}

