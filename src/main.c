#include "Gap8.h" 
#include "rt/rt_api.h"
#include "decoder.h"
#include "inputSignalMin.h"
#include "../lib/kiss_fft.h"
#include "../lib/_kiss_fft_guts.h"
#include "service.h"
#define NUMBER_OF_SAMPLES 47360 
#define TRIGGER (17)

//#define DETECT_DEBUG
//#define DEBUG_DEMOD

static void mureceiver(PTTPackage_Typedef * wpckg[NUMBER_OF_DECODERS])
{
	rt_freq_set(RT_FREQ_DOMAIN_CL,  175000000);
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
	int n0,iCh,ii;

	demodArg_t * d_ptr = rt_alloc(MEM_ALLOC, sizeof(demodArg_t));
//	sampler memory
//	sampler_mem * str_smp[NUMBER_OF_DECODERS];
	for (iCh = 0; iCh < NUMBER_OF_DECODERS; ++iCh){
		d_ptr->str_smp[iCh] = rt_alloc(MEM_ALLOC,sizeof(sampler_mem));
		d_ptr->str_smp[iCh]->smplBuffer = rt_alloc(MEM_ALLOC,2*smplPerSymb*(sizeof(int)));
	}

//	CIC filter of demod
//	mem_cic * str_cic[NUMBER_OF_DECODERS];
	for (iCh = 0; iCh < NUMBER_OF_DECODERS; ++iCh){
		d_ptr->str_cic[iCh] = rt_alloc(MEM_ALLOC,sizeof(mem_cic));
		d_ptr->str_cic[iCh]->previousAccRe = rt_alloc(MEM_ALLOC,delayIdx*sizeof(int));
		d_ptr->str_cic[iCh]->previousAccIm = rt_alloc(MEM_ALLOC,delayIdx*sizeof(int));
	}
  
//	CIC filter of sampler
//	mem_cic * str_cicSmp[NUMBER_OF_DECODERS];
	for (iCh = 0; iCh < NUMBER_OF_DECODERS; ++iCh){
		d_ptr->str_cicSmp[iCh]= rt_alloc(MEM_ALLOC,sizeof(mem_cic));
		d_ptr->str_cicSmp[iCh]->previousAccRe = rt_alloc(MEM_ALLOC,delaySmp*sizeof(int));
		d_ptr->str_cicSmp[iCh]->previousAccIm = rt_alloc(MEM_ALLOC,delaySmp*sizeof(int));
	}

//	demod_mem stores accumulators, symbOut and symbLock
//	demod_mem * str_demod[NUMBER_OF_DECODERS];
	for(iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
		d_ptr->str_demod[iCh] = rt_alloc(MEM_ALLOC,sizeof(demod_mem));
		d_ptr->str_demod[iCh]->symbLock = rt_alloc(MEM_ALLOC,nSymb*sizeof(int));
		d_ptr->str_demod[iCh]->symbOut = rt_alloc(MEM_ALLOC,nSymb*sizeof(int));
	}

//	This struct is used to control the FSM and show bit results
/*	PTTPackage_Typedef * wpckg[NUMBER_OF_DECODERS];
	for(iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
		wpckg[iCh] = rt_alloc(MEM_ALLOC,sizeof(PTTPackage_Typedef));
	}
*/
//	struct stores the interface (detect to demod) parameters
	FreqsRecord_Typedef *PTT_DP_LIST[NUMBER_OF_DECODERS];
	for(iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
		PTT_DP_LIST[iCh] = rt_alloc(MEM_ALLOC,sizeof(FreqsRecord_Typedef));
	}

//	Array to store the detected bins of last window	
	int * prevIdx = rt_alloc(MEM_ALLOC,DFT_LENGTH*sizeof(int));
	for(ii=0;ii<DFT_LENGTH;ii++){
		prevIdx[ii]=0;
	}


	int complex * inputSignal = rt_alloc(MEM_ALLOC,WINDOW_LENGTH*sizeof(int complex));
	d_ptr->vgaExp = rt_alloc(MEM_ALLOC,NUMBER_OF_DECODERS*sizeof(int));
	d_ptr->vgaMant = rt_alloc(MEM_ALLOC,NUMBER_OF_DECODERS*sizeof(int));
	d_ptr->InitFreq = rt_alloc(MEM_ALLOC,NUMBER_OF_DECODERS*sizeof(int));
	d_ptr->activeList = 0;

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
	for(iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
//		printf("Clearing decoder %d\n",iCh);
		clearDecoder(PTT_DP_LIST[iCh],wpckg[iCh], d_ptr->str_cic[iCh], d_ptr->str_cicSmp[iCh], d_ptr->str_smp[iCh], d_ptr->str_demod[iCh]);
	}
#endif

#ifdef DETECT_DEBUG
	for(iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
		PTT_DP_LIST[iCh]->detect_state = FREQ_NONE;
		PTT_DP_LIST[iCh]->freq_amp = 0;
		PTT_DP_LIST[iCh]->freq_idx = 0;
		PTT_DP_LIST[iCh]->timeout = 0;	
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
//		printf("%f %f \n",inputSignal[0]);
//		rt_gpio_set_pin_value(0,TRIGGER,1);
//		printf("Updating Timeout !\n");
		UpdateTimeout(PTT_DP_LIST,wpckg);

#ifndef DEBUG_DEMOD
//		printf("Starting detection loop ! %d us\n", detect_time);
		tmp0 =  detectLoop(inputSignal, prevIdx, PTT_DP_LIST);   
//		rt_gpio_set_pin_value(0,TRIGGER,0);

//		Setup Parameters: Frequency, Gain, Controls status of pckg and Detect.
		for (iCh=0;iCh<3;iCh++){
			if(PTT_DP_LIST[iCh]->detect_state==FREQ_DETECTED_TWICE){
				vga = VgaGain(PTT_DP_LIST[iCh]->freq_amp);
				d_ptr->vgaExp[iCh] = -1*(vga&0x3F);
				d_ptr->vgaMant[iCh] = (vga>>6)&0xFF;
				d_ptr->InitFreq[iCh] = PTT_DP_LIST[iCh]->freq_idx<<9;
//				printf("[%d]: mant %d exp %d freq %d\n",iCh, d_ptr->vgaMant[iCh],d_ptr->vgaExp[iCh],PTT_DP_LIST[iCh]->freq_idx);
				PTT_DP_LIST[iCh]->detect_state=FREQ_DECODING;
				wpckg[iCh]->status=PTT_FRAME_SYNCH;
				wpckg[iCh]->carrierFreq=PTT_DP_LIST[iCh]->freq_idx;
				wpckg[iCh]->carrierAbs=PTT_DP_LIST[iCh]->freq_amp;
				d_ptr->activeList++;
//				printf("%d %d \n", wpckg[iCh]->carrierFreq,wpckg[iCh]->carrierAbs);
			}
		}
#endif
int A0;
//#ifdef DEBUG_DEMOD
//		DEBUG DEMOD AND DECOD PROCESSES  
		if(nWind==1 && debug==0){
			for(int iprl=2;iprl<8;++iprl){
				A0 = iprl%2;
				PTT_DP_LIST[iprl]->detect_state = FREQ_DECODING;
				PTT_DP_LIST[iprl]->freq_amp = PTT_DP_LIST[A0]->freq_amp;
				PTT_DP_LIST[iprl]->freq_idx = PTT_DP_LIST[A0]->freq_idx;
				PTT_DP_LIST[iprl]->timeout = 100;
				vga = VgaGain(PTT_DP_LIST[iprl]->freq_amp);
				d_ptr->vgaExp[iprl] = -1*(vga&0x3F);
				d_ptr->vgaMant[iprl] = (vga>>6)&0xFF;
				d_ptr->InitFreq[iprl] = PTT_DP_LIST[A0]->freq_idx<<9;
 //				printf("[%d]: mant %d exp %d\n",0, vgaMant[0],vgaExp[0]);
				wpckg[iprl]->status=PTT_FRAME_SYNCH;
				wpckg[iprl]->carrierFreq= PTT_DP_LIST[A0]->freq_idx;
				wpckg[iprl]->carrierAbs= PTT_DP_LIST[A0]->freq_amp;
				d_ptr->activeList++;
			}
		}
//#endif

#ifndef DETECT_DEBUG
//		decodes signals from active channels
		decoder(inputSignal, d_ptr, PTT_DP_LIST, wpckg);
#endif 
	}//END SCROLLING WINDOWS
}//END FOR NSIM
	
	rt_gpio_set_pin_value(0,TRIGGER,0);

	rt_free(MEM_ALLOC,prevIdx,DFT_LENGTH*sizeof(int));
	rt_free(MEM_ALLOC,inputSignal,WINDOW_LENGTH*sizeof(int complex));
	rt_free(MEM_ALLOC,d_ptr->vgaExp,NUMBER_OF_DECODERS*sizeof(int));
	rt_free(MEM_ALLOC,d_ptr->vgaMant,NUMBER_OF_DECODERS*sizeof(int));
	rt_free(MEM_ALLOC,d_ptr->InitFreq,NUMBER_OF_DECODERS*sizeof(int));
  
	for(iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
		rt_free(MEM_ALLOC,PTT_DP_LIST[iCh],sizeof(FreqsRecord_Typedef));
    
#ifndef DETECT_DEBUG
		rt_free(MEM_ALLOC,d_ptr->str_smp[iCh]->smplBuffer,2*smplPerSymb*(sizeof(int)));   
		rt_free(MEM_ALLOC,d_ptr->str_smp[iCh],sizeof(sampler_mem));       
		rt_free(MEM_ALLOC,d_ptr->str_cicSmp[iCh]->previousAccRe,delaySmp*sizeof(int));
		rt_free(MEM_ALLOC,d_ptr->str_cicSmp[iCh]->previousAccIm,delaySmp*sizeof(int));
		rt_free(MEM_ALLOC,d_ptr->str_cicSmp[iCh],sizeof(mem_cic));
		rt_free(MEM_ALLOC,d_ptr->str_cic[iCh]->previousAccRe,delayIdx*sizeof(int));
		rt_free(MEM_ALLOC,d_ptr->str_cic[iCh]->previousAccIm,delayIdx*sizeof(int));
		rt_free(MEM_ALLOC,d_ptr->str_cic[iCh],sizeof(mem_cic));        
		rt_free(MEM_ALLOC,d_ptr->str_demod[iCh]->symbLock,nSymb*sizeof(int));
		rt_free(MEM_ALLOC,d_ptr->str_demod[iCh]->symbOut,nSymb*sizeof(int));
		rt_free(MEM_ALLOC,d_ptr->str_demod[iCh],sizeof(demod_mem));    
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

	rt_freq_set(RT_FREQ_DOMAIN_FC,250000000);
	printf("Entering main controller \n");
	int t_time = rt_time_get_us();  

	if (rt_event_alloc(NULL, 4)) return -1;

	rt_event_t *p_event = rt_event_get(NULL, end_of_call, (void *) CID);

	rt_cluster_mount(MOUNT, CID, 0, NULL);
	
	PTTPackage_Typedef * wpckg[NUMBER_OF_DECODERS];
	for(int iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
		wpckg[iCh] = rt_alloc(MEM_ALLOC,sizeof(PTTPackage_Typedef));
	}
	
	void *stacks = rt_alloc(MEM_ALLOC, STACK_SIZE);
	if(stacks == NULL) return -1;
//	rt_cluster_call(NULL, CID, entry, entry_args, stacks, master_stack_size, slave_stack_size, nb_cores, event)
	rt_cluster_call(NULL, CID, (void *)mureceiver, (void *)wpckg, stacks, 2400, 1400,8, p_event);
//	rt_cluster_call(NULL, CID, cluster_entry, NULL, stacks, STACK_SIZE>>1, STACK_SIZE>>2,rt_nb_pe(), p_event);

	while(!done){
		rt_event_execute(NULL, 1);
	}


	t_time = rt_time_get_us()-t_time;
	printf("Total time: %d.%d ms\n",t_time/1000,(t_time-((t_time/1000)*1000))/10);
	for(int iCh = 0;iCh<NUMBER_OF_DECODERS;++iCh){
		if(wpckg[iCh]->status==PTT_READY){
		printf("\n");
	  		printf("decoder_id: %d\n",iCh);
			printf("freq:\tamp:\tlength:\n");
			printf("%d\t%d\t%d\n",wpckg[iCh]->carrierFreq,wpckg[iCh]->carrierAbs,wpckg[iCh]->msgByteLength);
			printf("User MSG: \n");
			for(int i0=0;i0<wpckg[iCh]->msgByteLength;i0++){
				printf("%x ",wpckg[iCh]->userMsg[i0]);
			}
			printf("\n");
		}
	}

	for(int iCh=0;iCh<NUMBER_OF_DECODERS;iCh++){
		rt_free(MEM_ALLOC,wpckg[iCh],sizeof(PTTPackage_Typedef));
	}

	rt_cluster_mount(UNMOUNT, CID, 0, NULL);

	printf("\nTest success: Leaving main controller\n");
	return 0;
}
