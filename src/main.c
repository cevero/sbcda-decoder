#include "Gap8.h" 
#include "rt/rt_api.h"
#include "decoder.h"
#include "inputSignalMin.h"
#include "../lib/kiss_fft.h"
#include "../lib/_kiss_fft_guts.h"
#include "service.h"
#define NUMBER_OF_SAMPLES 47360 
#define TRIGGER (17)
#define MHz	(1000000)
//#define DETECT_DEBUG
//#define DEBUG_DEMOD

static void mureceiver(PTTPackage_T * outputPckg[NoD])
{
	rt_freq_set(RT_FREQ_DOMAIN_CL,  175*MHz);
//**************************GPIO Setup*****************************************
	rt_padframe_profile_t *profile_gpio = rt_pad_profile_get("hyper_gpio");
	if(profile_gpio == NULL){
		printf("pad config error\n");
//		return 1;
	}
	rt_padframe_set(profile_gpio);

//	GPIO Init
	rt_gpio_init(0,TRIGGER);
//	Config TRIGGER pin as an output
	rt_gpio_set_dir(0,1<<TRIGGER, RT_GPIO_IS_OUT);
	rt_gpio_set_pin_value(0,TRIGGER,1);

//**********************Variables Allocation************************************
	int n0,dId;//dId: decoder ID
	int ii;

	demodArg_t * arg = rt_alloc(MEM_ALLOC, sizeof(demodArg_t));
//	sampler memory
//	sampler_mem * str_smp[NoD];
	for (dId = 0; dId < NoD; ++dId){
		arg->str_smp[dId] = rt_alloc(MEM_ALLOC,sizeof(sampler_mem));
		arg->str_smp[dId]->smplBuffer = rt_alloc(MEM_ALLOC,2*smplPerSymb*(sizeof(int)));
	}

//	CIC filter of demod
//	mem_cic * str_cic[NoD];
	for (dId = 0; dId < NoD; ++dId){
		arg->str_cic[dId] = rt_alloc(MEM_ALLOC,sizeof(mem_cic));
		arg->str_cic[dId]->previousAccRe = rt_alloc(MEM_ALLOC,delayIdx*sizeof(int));
		arg->str_cic[dId]->previousAccIm = rt_alloc(MEM_ALLOC,delayIdx*sizeof(int));
	}
  
//	CIC filter of sampler
//	mem_cic * str_cicSmp[NoD];
	for (dId = 0; dId < NoD; ++dId){
		arg->str_cicSmp[dId]= rt_alloc(MEM_ALLOC,sizeof(mem_cic));
		arg->str_cicSmp[dId]->previousAccRe = rt_alloc(MEM_ALLOC,delaySmp*sizeof(int));
		arg->str_cicSmp[dId]->previousAccIm = rt_alloc(MEM_ALLOC,delaySmp*sizeof(int));
	}

//	demod_mem stores accumulators, symbOut and symbLock
//	demod_mem * str_demod[NoD];
	for(dId=0;dId<NoD;dId++){
		arg->str_demod[dId] = rt_alloc(MEM_ALLOC,sizeof(demod_mem));
		arg->str_demod[dId]->symbLock = rt_alloc(MEM_ALLOC,nSymb*sizeof(int));
		arg->str_demod[dId]->symbOut = rt_alloc(MEM_ALLOC,nSymb*sizeof(int));
	}

//	This struct is used to control the FSM and show bit results
//	PTTService_T * wpckg[NoD];
	for(dId=0;dId<NoD;dId++){
		arg->wpckg[dId] = rt_alloc(MEM_ALLOC,sizeof(PTTService_T));
	}

//	struct stores the interface (detect to demod) parameters
	FreqsRecord_T *PTT_DP_LIST[NoD];
	for(dId=0;dId<NoD;dId++){
		PTT_DP_LIST[dId] = rt_alloc(MEM_ALLOC,sizeof(FreqsRecord_T));
	}

//	Array to store the detected bins of last window	
	int * prevIdx = rt_alloc(MEM_ALLOC,DFT_LENGTH*sizeof(int));
	for(ii=0;ii<DFT_LENGTH;ii++){
		prevIdx[ii]=0;
	}

	int complex * inputSignal = rt_alloc(MEM_ALLOC,WINDOW_LENGTH*sizeof(int complex));
	arg->vgaExp = rt_alloc(MEM_ALLOC,NoD*sizeof(int));
	arg->vgaMant = rt_alloc(MEM_ALLOC,NoD*sizeof(int));
	arg->InitFreq = rt_alloc(MEM_ALLOC,NoD*sizeof(int));
	arg->activeList = 0;

	int tmp0, f=0,vga, activeList, nWind, spWind,iSymb,i0,i2;
	int detect_time=0, demod_time=0, decod_time=0, total_time=0,aux_time, decod_per_channel=0;
//#ifdef DEBUG_DEMOD
	int debug = 0;
//#endif

/*
//	Calculate LUT of Twiddle factors for fft computation-----*
	cpx * we = rt_alloc(MEM_ALLOC,1024*sizeof(cpx));//	-*
	for(i=0;i<DFT_LENGTH/2;i++){//				-*
		we[i].r=(cos(2*PI*i/DFT_LENGTH));//		-*
		we[i].i=-1*(sin(2*PI*i/DFT_LENGTH));//		-*
	}//------------------------------------------------------*
*/

//	printf("***------------ ALL READY --------------***\n");
#ifndef DETECT_DEBUG
	for(dId=0;dId<NoD;dId++){
//		printf("Clearing decoder %d\n",dId);
		clearDecoder(PTT_DP_LIST[dId],arg->wpckg[dId], arg->str_cic[dId], arg->str_cicSmp[dId], arg->str_smp[dId], arg->str_demod[dId]);
	}
#endif

#ifdef DETECT_DEBUG
	for(dId=0;dId<NoD;dId++){
		PTT_DP_LIST[dId]->detect_state = FREQ_NONE;
		PTT_DP_LIST[dId]->freq_amp = 0;
		PTT_DP_LIST[dId]->freq_idx = 0;
		PTT_DP_LIST[dId]->timeout = 0;	
	}
#endif

#define NSIM (1)
  
for(n0=0; n0<NSIM;n0++){
//printf("N_SIM = %d\n",n0);
#ifdef DEBUG_DEMOD
  if(n0==1)
   debug = 1;
#endif

	for (nWind=0;nWind<NUMBER_OF_SAMPLES/WINDOW_LENGTH;nWind++){
//		aux_time = rt_time_get_us();
//		printf("***--------- Window processing ---------*** [%d]\n", nWind);  
// 		Performs input partitioning on the windows of 1280 samples
//		printf("--------> Loading Input Signal %d <-------\n\n",rt_freq_get(RT_FREQ_DOMAIN_CL));
		memcpy(inputSignal,inputSignalMin+WINDOW_LENGTH*nWind,1280*sizeof(int complex));
/*		for(ii=0;ii<WINDOW_LENGTH;++ii){
			arg->inputSignalRe[ii] = creal(inputSignalMin[WINDOW_LENGTH*nWind+ii]);
			arg->inputSignalIm[ii] = cimag(inputSignalMin[WINDOW_LENGTH*nWind+ii]);
		}*/
//		printf("%f %f \n",inputSignal[0]);
//		rt_gpio_set_pin_value(0,TRIGGER,1);
//		printf("Updating Timeout !\n");
		UpdateTimeout(PTT_DP_LIST,arg->wpckg);

#ifndef DEBUG_DEMOD
//		printf("Starting detection loop ! %d us\n", detect_time);
		tmp0 =  detectLoop(inputSignal, prevIdx, PTT_DP_LIST);   
//		rt_gpio_set_pin_value(0,TRIGGER,0);

//		Setup Parameters: Frequency, Gain, Controls status of pckg and Detect.
		for (dId=0;dId<2;dId++){
			if(PTT_DP_LIST[dId]->detect_state==FREQ_DETECTED_TWICE){
				vga = VgaGain(PTT_DP_LIST[dId]->freq_amp);
				arg->vgaExp[dId] = -1*(vga&0x3F);
				arg->vgaMant[dId] = (vga>>6)&0xFF;
				arg->InitFreq[dId] = PTT_DP_LIST[dId]->freq_idx<<9;
//				printf("[%d]: mant %d exp %d freq %d\n",dId, arg->vgaMant[dId],arg->vgaExp[dId],PTT_DP_LIST[dId]->freq_idx);
				PTT_DP_LIST[dId]->detect_state=FREQ_DECODING;
				arg->wpckg[dId]->status=PTT_FRAME_SYNCH;
				arg->wpckg[dId]->carrierFreq=PTT_DP_LIST[dId]->freq_idx;
				arg->wpckg[dId]->carrierAbs=PTT_DP_LIST[dId]->freq_amp;
				arg->activeList++;
//				printf("%d %d \n", arg->wpckg[dId]->carrierFreq,arg->wpckg[dId]->carrierAbs);
			}
		}
#endif
int A0;
#ifdef DEBUG_DEMOD
//		DEBUG DEMOD AND DECOD PROCESSES  
		if(nWind==1 && debug==0){
			for(int iprl=2;iprl<2;++iprl){
				A0 = iprl%2;
				PTT_DP_LIST[iprl]->detect_state = FREQ_DECODING;
				PTT_DP_LIST[iprl]->freq_amp = PTT_DP_LIST[A0]->freq_amp;
				PTT_DP_LIST[iprl]->freq_idx = PTT_DP_LIST[A0]->freq_idx;
				PTT_DP_LIST[iprl]->timeout = 100;
				vga = VgaGain(PTT_DP_LIST[iprl]->freq_amp);
				arg->vgaExp[iprl] = -1*(vga&0x3F);
				arg->vgaMant[iprl] = (vga>>6)&0xFF;
				arg->InitFreq[iprl] = PTT_DP_LIST[A0]->freq_idx<<9;
 //				printf("[%d]: mant %d exp %d\n",0, vgaMant[0],vgaExp[0]);
				arg->wpckg[iprl]->status=PTT_FRAME_SYNCH;
				arg->wpckg[iprl]->carrierFreq= PTT_DP_LIST[A0]->freq_idx;
				arg->wpckg[iprl]->carrierAbs= PTT_DP_LIST[A0]->freq_amp;
				arg->activeList = (arg->activeList<<1)|1;
			}
		}
#endif

//	rt_freq_set(RT_FREQ_DOMAIN_CL,  100*MHz);
#ifndef DETECT_DEBUG
//		decodes signals from active channels
		decoder(inputSignal,arg, PTT_DP_LIST, outputPckg);
//	rt_freq_set(RT_FREQ_DOMAIN_CL, 175*MHz);
#endif 
	}//END SCROLLING WINDOWS
}//END FOR NSIM
	
	rt_gpio_set_pin_value(0,TRIGGER,0);

	rt_free(MEM_ALLOC,prevIdx,DFT_LENGTH*sizeof(int));
	rt_free(MEM_ALLOC,inputSignal,WINDOW_LENGTH*sizeof(int complex));
	rt_free(MEM_ALLOC,arg->vgaExp,NoD*sizeof(int));
	rt_free(MEM_ALLOC,arg->vgaMant,NoD*sizeof(int));
	rt_free(MEM_ALLOC,arg->InitFreq,NoD*sizeof(int));
  
	for(dId=0;dId<NoD;dId++){
		rt_free(MEM_ALLOC,PTT_DP_LIST[dId],sizeof(FreqsRecord_T));
    		rt_free(MEM_ALLOC,arg->wpckg[dId],sizeof(PTTService_T));
#ifndef DETECT_DEBUG
		rt_free(MEM_ALLOC,arg->str_smp[dId]->smplBuffer,2*smplPerSymb*(sizeof(int)));   
		rt_free(MEM_ALLOC,arg->str_smp[dId],sizeof(sampler_mem));       
		rt_free(MEM_ALLOC,arg->str_cicSmp[dId]->previousAccRe,delaySmp*sizeof(int));
		rt_free(MEM_ALLOC,arg->str_cicSmp[dId]->previousAccIm,delaySmp*sizeof(int));
		rt_free(MEM_ALLOC,arg->str_cicSmp[dId],sizeof(mem_cic));
		rt_free(MEM_ALLOC,arg->str_cic[dId]->previousAccRe,delayIdx*sizeof(int));
		rt_free(MEM_ALLOC,arg->str_cic[dId]->previousAccIm,delayIdx*sizeof(int));
		rt_free(MEM_ALLOC,arg->str_cic[dId],sizeof(mem_cic));        
		rt_free(MEM_ALLOC,arg->str_demod[dId]->symbLock,nSymb*sizeof(int));
		rt_free(MEM_ALLOC,arg->str_demod[dId]->symbOut,nSymb*sizeof(int));
		rt_free(MEM_ALLOC,arg->str_demod[dId],sizeof(demod_mem));    
#endif
	}

//	printf("---------------> Check <-------------------\n\n");  
}

#define STACK_SIZE	4096
#define MOUNT           1
#define UNMOUNT         0
#define CID             0
unsigned int done = 0;
static int alloc_size = 512;
static void hello(void *arg)
{
	printf("[clusterID: 0x%2x] Hello from core %d\n", rt_cluster_id(), rt_core_id());
}

static void cluster_entry(void *arg)
{
	printf("Entering cluster on core %d\n", rt_core_id());
	printf("There are %d cores available here.\n", rt_nb_pe());
	rt_team_fork(8, hello, (void *)0x0);
	printf("Leaving cluster on core %d\n", rt_core_id());
}

static void end_of_call(void *arg)
{
//	printf("[clusterID: 0x%x] MUR from core %d\n", rt_cluster_id(), rt_core_id());
	done = 1;
}

int main()
{
//	rt_event_sched_t * p_sched = rt_event_internal_sched();

	rt_freq_set(RT_FREQ_DOMAIN_FC,250*MHz);
	printf("Entering main controller \n");
	int t_time = rt_time_get_us();  

	if (rt_event_alloc(NULL, 4)) return -1;

	rt_event_t *p_event = rt_event_get(NULL, end_of_call, (void *) CID);

	rt_cluster_mount(MOUNT, CID, 0, NULL);
	
	PTTPackage_T * outputPckg[NoD];
	for(int dId=0;dId<NoD;dId++){
		outputPckg[dId] = rt_alloc(RT_ALLOC_FC_DATA,sizeof(PTTPackage_T));
	}
	
	void *stacks = rt_alloc(MEM_ALLOC, STACK_SIZE);
	if(stacks == NULL) return -1;
//	rt_cluster_call(NULL, CID, entry, entry_args, stacks, master_stack_size, slave_stack_size, nb_cores, event)
	rt_cluster_call(NULL, CID, (void *)mureceiver, (void *)outputPckg, stacks, 2400,1400,8, p_event);

	while(!done){
		rt_event_execute(NULL, 1);
	}

	t_time = rt_time_get_us()-t_time;
	printf("Total time: %d.%d ms\n",t_time/1000,(t_time-((t_time/1000)*1000))/10);
	for(int dId = 0;dId<NoD;++dId){
		if(outputPckg[dId]->status==PTT_READY){
		printf("\n");
	  		printf("decoder_id: %d\n",dId);
			printf("freq:\tamp:\tlength:\n");
			printf("%d\t%d\t%d\n",outputPckg[dId]->carrierFreq,outputPckg[dId]->carrierAbs,outputPckg[dId]->msgByteLength);
			printf("User MSG: \n");
			for(int i0=0;i0<outputPckg[dId]->msgByteLength;i0++){
				printf("%x ",outputPckg[dId]->userMsg[i0]);
			}
			printf("\n");
		}
	}

	for(int dId=0;dId<NoD;dId++){
		rt_free(RT_ALLOC_FC_DATA,outputPckg[dId],sizeof(PTTPackage_T));
	}

	rt_cluster_mount(UNMOUNT, CID, 0, NULL);

	printf("\nTest success: Leaving main controller\n");
	return 0;
}
