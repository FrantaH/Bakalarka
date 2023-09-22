/*----------------------------------------------------------------------------*/
/* DESCRIPTION:         Common audio streaming definitions
/* AUTHOR:              Petr Frenstatsky
/* URL:                 www.audified.com
/* COPYRIGHT:		2016-2017 AUDIFIED, All Rights Reserved
/*----------------------------------------------------------------------------*/

#ifndef AUDIOSTREAMER_DEFINITIONS_H_
#define AUDIOSTREAMER_DEFINITIONS_H_

#include <stdint.h>

//************** GENERAL PARAMETERS *******************

#define SAMPLE_RATE   			(48000u)		// Sample rate of streaming system
#define SAMPLE_SIZE 			(4u)			// Size of streamed samples
#define NUM_CHANNELS			(16u)			// Number of channels that will be transmitted
#define SAMPLES_PER_BUFFER		(256u)			// Number of samples stored in audio buffers
#define FILENAME_LEN (256)
//#define FOUT_BUFF_SIZE (50)

/* Macro to set buffer size */
#define AUDIO_BUFFER_SIZE 	        (SAMPLES_PER_BUFFER * NUM_CHANNELS * SAMPLE_SIZE)
#define RMS_INFO_PERIOD_S		(0.01f)

//Offsets of DDR addresses
#define DMC0_DATA_CALIB_ADD 0x80000000
#define DMC1_DATA_CALIB_ADD 0xC0000000

#define MCAPI_DOMAIN	0u
#define NODE_CORE_0	1u
#define NODE_ARM	0u

//#define TDM_MODE 1

//************** TRANSMISMISION PARAMETERS *******************

#define SAMPLES_PER_PACKET	(128u)						//Number of samples that are trasmitted via network
#define MESSAGETYPE_BYTES	(1u)						//Number of bytes for message type
#define TIMESTAMP_BYTES		(4u)						//Number of bytes of timestamp contained in packed
#define NSAMPLE_BYTES		(4u)						//Number of bytes of number of contained in packed in byte
#define NCHANNELS_BYTES		(1u)						//Number of bytes of channel count identificator in packet
#define NUM_AUDIO_BLOCKS	(128u)						//Size of circle buffer that contains audio buffers from SHARC Core 0
#define BLOCK_SIZE		(SAMPLES_PER_PACKET*NUM_CHANNELS*SAMPLE_SIZE)	//SIze of block that will be transmitted in packed
#define NUM_BLOCK_IN_AUDIO	(AUDIO_BUFFER_SIZE/BLOCK_SIZE)			//Number of block necessery to cover size of audio buffers passed from SHARC Core 0
#define NUMBER_AUDIO_BUFFERS	(2u)						//Number of Ping Pong buffer
#define TIMER_BYTES		(4u)

//Format of packet {timestamp, number of samples, number of channels, samples}
#define PACKET_SIZE (MESSAGETYPE_BYTES + TIMESTAMP_BYTES + NSAMPLE_BYTES + NCHANNELS_BYTES + SAMPLES_PER_PACKET * NUM_CHANNELS * SAMPLE_SIZE) /*uint*/
#define INFORM_PACKET_SIZE (MESSAGETYPE_BYTES+SAMPLE_SIZE*NUM_CHANNELS+TIMER_BYTES)

#define port_number		7891

#define STR_VER			"0.1"

///Structures
enum EPingPongBuff {
	kPing_buff = 0,
	kPong_buff
};

//running modes
enum kRunningMode {
	kMode_Streaming,
	kMode_TransmisionTesting,
	kMode_sinStreaming
};

//General error codes
enum EErrCodes {
	kOk = 0,
	kErrMCAPIInit,
	kErrAlocDevError,
	kErrAlocError,
	kErrMapError,
	kErrFreeError,
	kConnectionTimeOutErr,
	kConnectionErr,
	kStreamingError,
	kSHARCNotConnected,
	kStreamerStartError,
	kNoNetworkClient,
	kFileError,
	kSPIError,
	kSoundCardError
};

//connection erroCodes
enum EConnectioneStates {
	kConnect_OK = 0u,
	kConnect_TimeOut,
	kConnect_Error,
};

//connection erroCodes
enum ConfigurationMessages {
	kRecord = 254u,
	kRecordPlay,
	kExit,
};

enum EGainValues {
	kGain_0,
	kGain_10,
	kGain_20,
	kGain_30,
	kGain_40,
	kGain_50,
	kGain_60,
};

//intecore circular buffer
typedef struct {
	uint32_t head;
	uint32_t tail;
	uint32_t pageSize; //in sizeof(char)
	uint32_t numberOfPages;
	uint32_t state;
	uint32_t dropSHARC;
	uint32_t paramChanged;
	int32_t channelGains[NUM_CHANNELS];
	uint32_t channelGainsMask[NUM_CHANNELS];
} SIntCore_CircBuff;

//streamer states
enum EStreamerStates {
	kStreamerInit = 0,
	kStreamerInitialized,
	kStreamerDisable,
	kStreamerDisabled,
	kStreamerRun,
	kStreamerRunning
};

enum ENetMessageType {
	kNetMessageData,
	kNetMessageCfg,
	kNetMessageInform,
	kNetMessageInformRMS,
	kNetMessageAck,
	kNetMessageStrConfig,
	kNetMessageGetStrConfig,
	kNetMessageAlive
};

//controlling IDs
enum kControlIDs {
	kStartControl,
	kRunControl,
	kGetMutes,
	kSetMutes,
	kSetGainFirst,
	kSetGainLast = kSetGainFirst + NUM_CHANNELS,
	kSetPhantomFirst,
	kSetPhantomLast = kSetPhantomFirst + NUM_CHANNELS,
};

typedef struct {
	int packetSize;
	int numChannels;
	int sampleRate;
	int alsaDriverIndex;

	/*SProgramCfg()
	{
	  packetSize = PACKET_SIZE;
	  numChannels = NUM_CHANNELS;
	  sampleRate = SAMPLE_RATE;
	  alsaDriverIndex = 1;
	}*/
} SProgramCfg;

#endif	/* AUDIOSTREAMER_DEFINITIONS_H_ */
