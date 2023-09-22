/*----------------------------------------------------------------------------*/
/* DESCRIPTION:         TODO
/* AUTHORS:             Petr Frenstatsky
/*                      Jan Havran
 * 						Martin Lach
/* URL:                 www.audified.com
/* COPYRIGHT:		2016-2019 AUDIFIED, All Rights Reserved
/*----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <mutex>
#include <thread>
#include <iostream>
#include <float.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <math.h>



#include "CAlsaStreamer.h"

#define FRAMES_PER_READ		32*NUM_CHANNELS
#define PCM_FORMAT		SND_PCM_FORMAT_FLOAT_LE

#define CARD_NAME		"hw:0,0"
#define DEBUG_LOG		0

#define WAVFILES_FILE "wavFile.txt"



/** Constructor
 * */
CAlsaStreamer::CAlsaStreamer(int _mode)
{
	mode = _mode;
	circBuff =NULL;

	errRate = 0;
	thread_sendData = NULL;
	sendingThreadRunning = false;
	streaming = false;
	localBufferSIze = 0;
	capture_handle = NULL;
	dataFlow = 0;
	soundCardName = NULL;

	for (int i=0; i<NUM_CHANNELS; i++) {
		RMS[i] = 0.f;
		gains[i] = 1.f;
		mutes[i] = false;
	}

}

/** Destructor
 * */
CAlsaStreamer::~CAlsaStreamer()
{
	/*if(capture_handle)
	snd_pcm_close (capture_handle);
	capture_handle = NULL;*/

	if (circBuff) {
		delete circBuff;
		circBuff = NULL;
	}

	if (localBuffer)
		delete[] localBuffer;
}

/** Initialization of audio streamer
 * Allocaton of memory blocks
 * @param pointer to network connection object
 * @return success

**/
int CAlsaStreamer::initStreamer(CNetworkConnection *_netConn)
{

	void *harm_buf, *buffer;
	int err = kOk;
	int retval = 0;
	int i;
	state = kStreamerInit;

	//*********Init MCAPI*************//
	printf("Initializing Audio Streamer\n\n");

	if (circBuff) {
		delete circBuff;
	}

	circBuff = new CCircBuffer();
	circBuff->setProperties(NUM_AUDIO_BLOCKS*NUM_BLOCK_IN_AUDIO, BLOCK_SIZE);

	if ((err =prepareBuffers())!=kOk)
		return err;

	networkConnection = _netConn;

	//probe sound card
	if ((err =initSoundCard(CARD_NAME)!=kOk)) {
		return err;
	}



	printf("Audio Streamer initialized\n");





	return err;
}

void CAlsaStreamer::sendData(CCircBuffer *circBuff, CNetworkConnection *_netConn, CAlsaStreamer *_streamer)
{
	int buffIndex = 0;
	int timestamp = 0;
	int index = 0;
	int ret = kOk;;
	int RMSPeriod = SAMPLE_RATE/SAMPLES_PER_PACKET;
	char *packet = new(std::nothrow) char[PACKET_SIZE];

	if (packet == NULL) {
		fprintf(stderr, "Packet allocation failed\n");
		return;
	}

//	printf("RMS inform period: %d\n",RMSPeriod);

	while (_streamer->sendingThreadRunning) {
		if (buffIndex>=circBuff->blockNumber)
			buffIndex = 0;

#if DEBUG_LOG
		printf("Waiting for %d\n",buffIndex % circBuff->blockNumber);
#endif

		if (circBuff->circBuffMtx_ready[buffIndex % circBuff->blockNumber].try_lock()) {
			circBuff->circBuffMtx_data[buffIndex % circBuff->blockNumber].lock();

			if (circBuff->buff_ready[buffIndex % circBuff->blockNumber] == true) {
				create_DataPacket((circBuff->buffer[buffIndex % circBuff->blockNumber]), timestamp++, SAMPLES_PER_PACKET, NUM_CHANNELS, packet);

#if DEBUG_LOG
				FILE* payload;
				payload = fopen("payload", "a");
				fwrite((circBuff->buffer[buffIndex % circBuff->blockNumber]), SAMPLE_SIZE, SAMPLES_PER_PACKET*NUM_CHANNELS,payload);
			   fclose(payload);
#endif

				ret = _netConn->sendPacket(packet, PACKET_SIZE);

#if DEBUG_LOG
				int* ptr = (int*)(circBuff->buffer[buffIndex % circBuff->blockNumber]);
				for (int i_s = 0; i_s < /*circBuff->blockSize / SAMPLE_SIZE*/1; i_s+=NUM_CHANNELS) {
					printf("S: %d: BufIndex: %d Val: %d\n",i_s, buffIndex % circBuff->blockNumber,ptr[i_s]);
				}
#endif

				circBuff->buff_ready[buffIndex % circBuff->blockNumber] = false;

				index++;
			} else {
				printf("Error - not enought lenght of circle buffer\n");
			}

			circBuff->circBuffMtx_data[buffIndex % circBuff->blockNumber].unlock();

#if DEBUG_LOG
			printf("Send %d\n",buffIndex % circBuff->blockNumber);
#endif


			buffIndex++;

			if (ret!=kOk) {
				_streamer->sendingThreadRunning = false;
				break;
			}
		}
	}

	delete[] packet;
	_streamer->state = kStreamerDisable;
	printf("Finishing sending thread\n");

	return;
}

/** Run main streamer loop
 * @return success
**/
int CAlsaStreamer::runStreaming()
{
	const int numBlocks = NUM_AUDIO_BLOCKS*NUM_BLOCK_IN_AUDIO;
	int err;

	if (sendingThreadRunning)
		return kOk;

	printf("Running streamer\n");

	printf("Executing running thread\n");

	//Run sendinf thread
	if (thread_sendData) {
		printf("Deleting thread_sendData\n");
		thread_sendData->join();
		delete thread_sendData;
	}

	printf("Running thread_sendData\n");
	sendingThreadRunning = true;
	thread_sendData = new std::thread(sendData, circBuff, networkConnection, this);
	recievedBlocks = 0;
	position = 0;
	streamingTime = 0;
	dataFlow = 0;
	gettimeofday(&lastClock, NULL);
	streaming = true;


	//snd_pcm_pause(capture_handle,0);

	int numParts = NUM_BLOCK_IN_AUDIO;
	printf("Running main loop %d, Num %d, BlockSIze %d\n",numBlocks, NUM_BLOCK_IN_AUDIO, circBuff->blockSize);
	printf("Streaming ...\n");

	state = kStreamerRunning;

	return kOk;
}

int CAlsaStreamer::stopStreaming()
{
	state = kStreamerDisable;
	streaming = false;

	return kOk;
}

/*
 * Creates the packet content and saves to the address specified by
 * packet pointer. It needs to be allocated to the packet_size!
 * @buffer - the audio data, see README for format
 * @timestamp - order number of the packet
 * @n_samples - # of samples stored in the packet
 * @n_c - # of channels
 * @packet - output, the packet content is stored here
 */
void CAlsaStreamer::create_DataPacket(char * buffer, uint32_t timestamp,
                                      uint32_t n_samples, uint8_t n_c,
                                      char * packet)
{
	char ID = kNetMessageData;
	memcpy(packet, &ID, MESSAGETYPE_BYTES);
	packet += MESSAGETYPE_BYTES;
	memcpy(packet, &timestamp, TIMESTAMP_BYTES);
	memcpy(packet+TIMESTAMP_BYTES, &n_samples, NSAMPLE_BYTES);
	memcpy(packet+TIMESTAMP_BYTES+NSAMPLE_BYTES, &n_c, NCHANNELS_BYTES);

	memcpy(packet+TIMESTAMP_BYTES+NSAMPLE_BYTES+NCHANNELS_BYTES, buffer,
	       n_samples * NUM_CHANNELS * SAMPLE_SIZE);

}
//Martin Lach
//sending Peak
void CAlsaStreamer::countRMS(sample_t value, int chan)
{
	float val = value;///DBL_MAX; //(gains[chan]*buffPtr[k*NUM_CHANNELS + chan])/32768.f;
	if (val<0)
		val*=-1;
	if (val>RMS[chan])
		RMS[chan] = val;
}
//Martin Lach
void CAlsaStreamer::sendConfigurationMessage(int code, char * string)
{
	char packet[PACKET_SIZE];
	memset(packet,0,PACKET_SIZE*sizeof(char));
	packet[0] = kNetMessageInform;
	*(uint32_t*)(packet+1) = code;
	memcpy(packet+5, string, FILENAME_LEN);

	networkConnection->sendPacket(packet, PACKET_SIZE);
}


//main loop
//Martin Lach
void CAlsaStreamer::idle()
{
	int r;

	r = snd_pcm_readi(capture_handle, localBuffer, SAMPLES_PER_PACKET); //actually reads  * NUM_CHANNELS
	#if DEBUG_LOG
		FILE* p;
		p = fopen("p", "a");
		//fprintf(payload,"%s",(circBuff->buffer[buffIndex % circBuff->blockNumber]));
		fwrite(localBuffer, SAMPLE_SIZE, SAMPLES_PER_PACKET*NUM_CHANNELS,p);
		fclose(p);
		//printf("Frames read: %d Peak chan 0 = %f\n",r, RMS[0]*100);
	 //printf("Precteno: %d\n",r);
	#endif

	if (r == -EPIPE) {
			printf("recovering\n");
			snd_pcm_recover(capture_handle, r, 1);
	} else if (r == -EIO) {
			printf("underflow\n");
			return;
	} else if (r == -ESTRPIPE) {
		state = kStreamerDisable;
		return;
	} else if (r < 0) {
		printf("read error: %s\n", snd_strerror(r));
		state = kStreamerDisable;
		return;
	}

	for (int chan = 0; chan < NUM_CHANNELS; chan++) {
	//RMS[chan]=0;
		for (int k=0; k< SAMPLES_PER_PACKET; k++) {
			//printf("now: %s ", destBuff[k*NUM_CHANNELS + chan]);
			CAlsaStreamer::countRMS(((sample_t*)localBuffer)[k*NUM_CHANNELS + chan], chan);
		}
	}

	switch (state) {
	case kStreamerDisable:
		printf("Stopping streamer\n");
		sendingThreadRunning = false;
		streaming = false;

		if (thread_sendData) {
			thread_sendData->join();
			delete thread_sendData;
			thread_sendData = NULL;
		}

		circBuff->resetAllMutexes();

		state = kStreamerDisabled;
		printf("Streamer stopped\n");
		break;
	case kStreamerRunning:
		if (!sendingThreadRunning) {
			stopStreaming();
			return;
		}



		if (recievedBlocks>=circBuff->blockNumber)
			recievedBlocks = 0;

#if DEBUG_LOG
		printf("Receiving %d - %d - %d\n",recievedBlocks % circBuff->blockNumber, recievedBlocks, circBuff->blockNumber);
#endif

		while (circBuff->buff_ready[recievedBlocks]) {
			if (state!=kStreamerRunning)
				break;
			printf("waiting\n");
		}

	while (!circBuff->circBuffMtx_data[recievedBlocks].try_lock()) {
		printf("waiting for buffer\n");
	}

		memcpy(circBuff->buffer[recievedBlocks],localBuffer,SAMPLES_PER_PACKET*SAMPLE_SIZE*NUM_CHANNELS);//n_samples * NUM_CHANNELS * SAMPLE_SIZE

		circBuff->buff_ready[recievedBlocks] = true;

		circBuff->circBuffMtx_data[recievedBlocks].unlock();
		circBuff->circBuffMtx_ready[recievedBlocks].unlock();

		recievedBlocks++;

		break;
	};
}

bool CAlsaStreamer::isStreaming()
{
	return streaming;
}

int CAlsaStreamer::prepareBuffers()
{
	localBuffer = new char[NUM_CHANNELS*SAMPLE_SIZE*FRAMES_PER_READ];
	return kOk;
}

int CAlsaStreamer::initSoundCard(const char* _drv)
{
	int i;
	int err;

	printf("Opening device\n");

	if ((err = snd_pcm_open (&capture_handle, _drv, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n", _drv, snd_strerror (err));
		return kSoundCardError;
	}

	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
		         snd_strerror (err));
		return kSoundCardError;
	}

	if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
		         snd_strerror (err));
		return kSoundCardError;
	}

	if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED  )) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
		         snd_strerror (err));
		return kSoundCardError;
	}

	if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, PCM_FORMAT)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
		         snd_strerror (err));
		return kSoundCardError;
	}

	unsigned int samplRate = SAMPLE_RATE;

	if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &samplRate, 0)) < 0) {
		fprintf (stderr, "cannot set sample rate (%s)\n",
		         snd_strerror (err));
		return kSoundCardError;
	}

	if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, NUM_CHANNELS)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",
		         snd_strerror (err));
		return kSoundCardError;
	}
		snd_pcm_uframes_t size = 10000;
	if ((err = snd_pcm_hw_params_set_buffer_size_max (capture_handle, hw_params, &size)) < 0) {
		fprintf (stderr, "cannot set buffer (%s)\n",
		         snd_strerror (err));
		return kSoundCardError;
	}
	else
		printf("Buffer size: %d\n", size);

	if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
		         snd_strerror (err));
		return kSoundCardError;
	}

	snd_pcm_hw_params_free (hw_params);

	if ((err = snd_pcm_prepare (capture_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
		         snd_strerror (err));
		return kSoundCardError;
	}

	if (snd_card_get_name(1, &soundCardName) != 0) {
		printf ("Name: %s\n", soundCardName);
	}

	printf("Sound card initialized\n");
	return kOk;
}

//TODO
void CAlsaStreamer::setMutes(uint32_t _mutes)
{
}

//TODO
void CAlsaStreamer::getMutes()
{
}

float* CAlsaStreamer::getPeaks()
{
	return RMS;
}
