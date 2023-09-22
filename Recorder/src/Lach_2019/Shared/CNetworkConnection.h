/*----------------------------------------------------------------------------*/
/* DESCRIPTION:         TCP/IP network library
/* AUTHORS:             Petr Frenstatsky
/*                      Jan Havran
/* URL:                 www.audified.com
/* COPYRIGHT:		2016-2019 AUDIFIED, All Rights Reserved
/*----------------------------------------------------------------------------*/

/**
 * 1) Reads RAW files specified in config.h (this should be replaced
 *      by reading from micarray)
 * 2) Splits the data into packets.
 * 3) Sends the packets over the network.
 */

#include <stdint.h>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <thread>

#include <iostream>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../Shared/AudioStreamer_definitions.h"

struct SNetDefinition {
	/*SNetDefinition()
	{
	  sendingSocket = -1;
	  recievingSocket = -1;
	  remoteController = -1;
	  newsockfd = -1;
	}*/

	struct sockaddr_in addr;		//Address of destination
	struct sockaddr_in addrRec;		//Address of reciever
	int sendingSocket;
	int recievingSocket;
	int newsockfd;
	struct sockaddr_storage serverStorage;
};

class CNetworkConnection
{
public:
	CNetworkConnection();
	~CNetworkConnection();

	int estabNetConnection(int port);
	int connectToServer(char* _IPAddress, int _port);

	int sendPacket(char* _data, int _size);
	int recieveMessage(char *_data, size_t _size, bool _waitAll = false);
	bool isConnected();
	int reconnect();

private:
	// int send(char * buffer, int n_samples);
	SNetDefinition netCfg;
	bool connected;
	bool checkConnection();
};

