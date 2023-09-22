/*----------------------------------------------------------------------------*/
/* DESCRIPTION:         TODO
/* AUTHORS:             Petr Frenstatsky
/*                      Jan Havran
 * 						Martin Lach
/* URL:                 www.audified.com
/* COPYRIGHT:		2016-2019 AUDIFIED, All Rights Reserved
/*----------------------------------------------------------------------------*/

#include "CStreamerController.h"

CStreamerController::CStreamerController()
{
	networkConnection = new CNetworkConnection();
	streamer = NULL;
	controllerState = kStateInit;
	//playingFile = "";
}

CStreamerController::~CStreamerController()
{
	delete networkConnection;

	if (streamer)
		delete streamer;

	if (thread_recieveData) {
		thread_recieveData->join();
		delete thread_recieveData;
	}
	if (thread_play) {
		thread_play->join();
		delete thread_play;
	}
}

/*
 * initialization of CStreamerController
 * Function estabilish connection with receiving node, creates thread for
 * recieved controll messages and finaly it initializes object of CStreamer class
 * which is used for a main communication with SHARC processor
 * @param _IPAddress address of recieving node
 * @param _port port of recieving node
 * @param _mode running mode of streamer
 * @return success
 */
int CStreamerController::init(int _mode)
{
	streamer = new CAlsaStreamer(_mode);
	RMS = streamer->getPeaks();
	playingFile.assign("");
	return kOk;
}


int CStreamerController::init2(int _port)
{
	int	err = networkConnection->estabNetConnection(_port);
	if (err == kOk) {
		err = streamer->initStreamer(networkConnection);
		thread_recieveData = new std::thread(recieveControllMessages, networkConnection, this);



		controllerState = kStateInitialized;
	}
	if(err != kOk)
		return err;

}

/*
 * Functions running in separated thread waits for incoming messages from network
 * @param pointer to network connection interface object
 * @param pointer to streamer object
 */
void CStreamerController::recieveControllMessages(CNetworkConnection *_netConn, CStreamerController *_streamer)
{
	if (!_netConn)
		return;

	int messageType = -1;
	char * recData = new char[PACKET_SIZE - MESSAGETYPE_BYTES];

	while (1) {
		if (_netConn->isConnected()) {
			messageType = _netConn->recieveMessage(recData, PACKET_SIZE - MESSAGETYPE_BYTES);
			printf("Recieved %i\n",messageType);

			if (messageType==-1)
				continue;

			_streamer->processMessage(messageType, recData);
		} else {
			_netConn->reconnect();
		}
	}

	delete[] recData;
}

/*
 * function processes incoming messages
 * @param id of incoming message
 * @param raw data of message
 */
void CStreamerController::processMessage(int _message,const char* _data)
{
	switch (_message) {
	case kNetMessageCfg:
		processControlMessages(*(uint32_t*)(&_data[1]),*(uint32_t*)(&_data[5]));
		printf("Recieved message: %d %d\n",*(uint32_t*)(&_data[1]),*(uint32_t*)(&_data[5]));

		break;
	case kNetMessageInform:
		break;
	case kNetMessageInformRMS:
		break;
	}
}


//edited by Martin Lach
//
void CStreamerController::processControlMessages(uint32_t _ID, uint32_t _value)
{
	switch (_ID) {
	case kRunControl:
		if (streamer) {
			if (_value){

				streamer->runStreaming();
				//streamer->sendConfigurationMessage(1);
				if(_value > 1)
					thread_play = new std::thread(play, streamer, &playingFile);
			}
			else{
				streamer->stopStreaming();
				thread_play->join();
			}
		}
		break;
	case kGetMutes:
		streamer->getMutes();
		break;
	case kSetMutes:
		streamer->setMutes(_value);
		break;
	}

}

/*
 * Neverending loop
 */
void CStreamerController::idle()
{
	while (1) {
		streamer->idle();
	};
}
/*
 * play thread -- Martin Lach
 */
void CStreamerController::play(CAlsaStreamer *_streamer, std::string* playing)
{
	char name[FILENAME_LEN] = "";
	//playing = name;
	std::ifstream playlistFile;
	playlistFile.open("files/playlists/.activeplaylist");


	bool played = false;

	if (playlistFile.is_open())
	{
		std::size_t lenght;
		std::string str;


		while(std::getline(playlistFile, str, ','))
		{
			std::string play_str = "aplay -D plughw:CARD=SHARC,DEV=0 files/audio/";
			//lenght = str.copy(playing, FILENAME_LEN, 0);
			//playing[lenght] = '\0';
			lenght = str.copy(name, FILENAME_LEN, 0);
			name[lenght] = '\0';
			playing->assign(str);
			play_str.append(str);
			play_str.append(".wav");



			if(!_streamer->isStreaming())
				break;
			_streamer->sendConfigurationMessage(1,name );
			system(play_str.c_str());
			_streamer->sendConfigurationMessage(0, name);


		}
	}
	playing->assign("");
	played = true;
	playlistFile.close();
	sleep(1);
	char text[FILENAME_LEN] = "Nahrávání dokončeno";
	_streamer->sendConfigurationMessage(kExit, text);
}


void CStreamerController::sendMessage(int code, char* string)
{
streamer->sendConfigurationMessage(code, string);
}
int CStreamerController::getState()
{
	return (controllerState);
}
int CStreamerController::isStreaming()
{
	return (streamer->isStreaming());
}
int CStreamerController::isConnected()
{
	return (networkConnection->isConnected());
}
