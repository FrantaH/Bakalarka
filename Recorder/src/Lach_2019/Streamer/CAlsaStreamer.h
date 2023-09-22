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
#include <alsa/asoundlib.h>
#include "../Shared/AudioStreamer_definitions.h"
#include "IStreamer.h"
#include <sys/time.h>


typedef float sample_t;


class CCircBuffer
{
public:
	CCircBuffer()
	{
		buffer = NULL;
		circBuffMtx_ready = NULL;
		circBuffMtx_data = NULL;
		buff_ready = NULL;
	}

	~CCircBuffer()
	{
		if (buffer) {
			int i;
			for (i = 0; i < blockNumber; i++) {
				delete[] buffer[i];
			}
			delete[] buffer;
		}

		if (circBuffMtx_ready) delete[] circBuffMtx_ready;
		if (circBuffMtx_data) delete[] circBuffMtx_data;
		if (buff_ready) delete[] buff_ready;
	}

	int setProperties(int _nBlock, int _blockSize)
	{
		int i;
		WP = 0;
		RP = 0;
		blockNumber = _nBlock;
		blockSize = _blockSize;
		circBuffMtx_ready = new std::mutex[blockNumber];
		circBuffMtx_data = new std::mutex[blockNumber];
		buff_ready = new bool[blockNumber];

		buffer = new char*[_nBlock];
		for (i = 0; i < blockNumber; i ++) {
			buffer[i] = new char[_blockSize];
			circBuffMtx_ready[i].lock();
			buff_ready[i] = false;
		}
	}

	void resetAllMutexes()
	{
		if (circBuffMtx_ready && circBuffMtx_data) {
			for (int i = 0; i < blockNumber; i ++) {
				circBuffMtx_ready[i].unlock();
				circBuffMtx_data[i].unlock();
				circBuffMtx_ready[i].lock();
				buff_ready[i] = false;
			}
		}
	}

	char** buffer;
	int size,WP,RP,blockNumber,blockSize;
	std::mutex *circBuffMtx_ready;
	std::mutex *circBuffMtx_data;
	bool* buff_ready;
};

class CAlsaStreamer : public IStreamer
{
public:
	CAlsaStreamer(int _mode);
	~CAlsaStreamer();

	int runStreaming();
	int stopStreaming();
	int initStreamer(CNetworkConnection *_netConn);
	void idle();
	bool isStreaming();
	void setMutes(uint32_t _mutes);
	void getMutes();
	void sendConfigurationMessage(int timestamp, char*string);
	float* getPeaks();

private:

	static void create_DataPacket(char * buffer, uint32_t timestamp,
	                              uint32_t n_samples, uint8_t n_c,
	                              char * packet);
	void createInformRMSPacket(char * packet);

	static void sendData(CCircBuffer *circBuff, CNetworkConnection *_netConn, CAlsaStreamer *_streamer);

	void countRMS(sample_t value, int chan);
	int prepareBuffers ();
	int initSoundCard(const char* _drv);
	CCircBuffer *circBuff;
	int mode;
	int currentIndex;
	int errRate;
	std::thread *thread_sendData;
	int recievedBlocks;
	bool sendingThreadRunning;
	bool streaming;
	int state;
	char *localBuffer;
	int localBufferSIze;
	int position;
	CNetworkConnection *networkConnection;
	snd_pcm_t *capture_handle;
	snd_pcm_hw_params_t *hw_params;
	float RMS[NUM_CHANNELS];
	bool mutes[NUM_CHANNELS];
	float gains[NUM_CHANNELS];
	int streamingTime;
	struct timeval lastClock;
	float dataFlow;
	char * soundCardName;
};

