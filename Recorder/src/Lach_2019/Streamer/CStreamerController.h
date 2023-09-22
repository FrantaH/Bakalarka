/*----------------------------------------------------------------------------*/
/* DESCRIPTION:         TODO
/* AUTHORS:             Petr Frenstatsky
/*                      Jan Havran
 * 						Martin Lach
/* URL:                 www.audified.com
/* COPYRIGHT:		2016-2019 AUDIFIED, All Rights Reserved
/*----------------------------------------------------------------------------*/

#include "CAlsaStreamer.h"


enum EControllerStates {
	kStateInit,
	kStateInitialized,
	kStateNoConnection,
	kStateConnect,
	kStateConnected,
	kStateRunning
};

class CStreamerController
{
private:

	CAlsaStreamer *streamer;
	CNetworkConnection *networkConnection;
	std::thread *thread_recieveData;
	std::thread *thread_play;
	int controllerState;

	void processMessage(int _message,const char* _data);
	void processControlMessages(uint32_t _ID, uint32_t _value);
	static void recieveControllMessages(CNetworkConnection *_netConn, CStreamerController *_streamer);

public:
	CStreamerController();
	~CStreamerController();
	int init(int _mode);
	int init2(int _port);
	float *RMS;
	int init(int _port, int _mode);
	void idle();
	static void play(CAlsaStreamer *_streamer, std::string*  playing);
	void sendMessage(int code, char* string);
	int getState();
	int isStreaming();
	int isConnected();
	std::string playingFile;

};

