/*
*	DDS_FreqsRecord_Typedef will be passed to the next processing
*/

typedef struct {
	uint32_t freq_idx;
	uint32_t freq_amp;
	uint8_t detect_state;
} DDS_FreqsRecord_Typedef;
/*
*	DDS_PassSet_Typedef assists in this processing
*/
typedef struct {
	uint32_t passIdx[DDS_DFT_LENGTH/DDS_NUMBER_OF_DECODERS];
	uint32_t passAmp[DDS_DFT_LENGTH/DDS_NUMBER_OF_DECODERS];
} DDS_PassSet_Typedef;

typedef enum {
	FREQ_NONE,
	FREQ_DETECTED_ONCE,
	FREQ_DETECTED_TWICE,
	FREQ_DECODING
} DDS_FreqsState_Typedef;

#define CONVERT_TO_32BIT_SIGNED(val, nbits)		(int) (val | (0xFFFFFFFF<<(nbits-1)))
#define TEST_SIGN_BIT(val, nbits)				((val & (0x00000001<<(nbits-1)))!=0)
#define DDS_FREQ_NUMBER_OF_BITS									11
#define DDS_NUMBER_OF_DECODERS									12