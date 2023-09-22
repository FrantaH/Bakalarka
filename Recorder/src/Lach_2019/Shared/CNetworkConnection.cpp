/*----------------------------------------------------------------------------*/
/* DESCRIPTION:         TCP/IP network library
/* AUTHORS:             Petr Frenstatsky
/*                      Jan Havran
/* URL:                 www.audified.com
/* COPYRIGHT:		2016-2019 AUDIFIED, All Rights Reserved
/*----------------------------------------------------------------------------*/

#include "CNetworkConnection.h"

enum EConnectionState {
	kConn_searching,
	kConn_connecting,
	kConn_connected
};

CNetworkConnection::CNetworkConnection()
{
	connected = false;
}

CNetworkConnection::~CNetworkConnection()
{
}

int CNetworkConnection::estabNetConnection(int port)
{
	// ------------------ SETUP OF THE NETWORK -------------------------
	int sockfd, newsockfd, portno;
	socklen_t clilen;

	netCfg.newsockfd = -1;

	netCfg.sendingSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (netCfg.sendingSocket < 0)
		printf("ERROR opening socket");

	bzero((char *) &netCfg.addr, sizeof(netCfg.addr));

	netCfg.addr.sin_family = AF_INET;
	netCfg.addr.sin_addr.s_addr = INADDR_ANY;
	netCfg.addr.sin_port = htons(port);

	if (bind(netCfg.sendingSocket, (struct sockaddr *) &netCfg.addr,
	         sizeof(netCfg.addr)) < 0)
		printf("ERROR on binding");

	listen(netCfg.sendingSocket,5);
	clilen = sizeof(netCfg.addrRec);
	//blocking until some connection is recieved
	printf("Waiting for connection on port %d\n",port);
	netCfg.newsockfd = accept(netCfg.sendingSocket,
	                          (struct sockaddr *) &netCfg.addrRec,
	                          &clilen);

	if (netCfg.newsockfd >= 0) {
		struct timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		setsockopt(netCfg.newsockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

		printf("Socket number %d\n",netCfg.newsockfd);
		connected = true;
		return kOk;
	} else {
		return kNoNetworkClient;
	}
}

int CNetworkConnection::connectToServer(char* _IPAddress, int _port)
{
	printf("Connecting to %s[%d] hostname port\n", _IPAddress, _port);

	netCfg.newsockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (netCfg.newsockfd < 0) {
		printf("ERROR opening socket\n");
	}

	bzero((char *) &netCfg.addr, sizeof(netCfg.addr));
	netCfg.addr.sin_family = AF_INET;
	netCfg.addr.sin_addr.s_addr = inet_addr(_IPAddress);
	netCfg.addr.sin_port = htons(_port);

	if (connect(netCfg.newsockfd,(struct sockaddr *) &netCfg.addr,sizeof(netCfg.addr)) < 0) {
		printf("Oh dear bear, something went wrong with Connect()! %s\n", strerror(errno));
		printf("ERROR connecting\n");
		return kNoNetworkClient;
	}

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	setsockopt(netCfg.newsockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	connected = true;

	return kOk;
}

int CNetworkConnection::reconnect()
{
	printf("Reconnecting\n");
	socklen_t clilen;

	listen(netCfg.sendingSocket,5);
	clilen = sizeof(netCfg.addrRec);
	//blocking until some connection is recieved
	netCfg.newsockfd = accept(netCfg.sendingSocket,
	                          (struct sockaddr *) &netCfg.addrRec,
	                          &clilen);

	if (netCfg.newsockfd >= 0) {
		struct timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		setsockopt(netCfg.newsockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

		printf("Socket number %d\n",netCfg.newsockfd);
		connected = true;
		return kOk;
	} else {
		return kNoNetworkClient;
	}
}

bool CNetworkConnection::checkConnection()
{
	int error = 0;
	socklen_t len = sizeof (error);
	int retval = getsockopt (netCfg.newsockfd, SOL_SOCKET, SO_ERROR, &error, &len);

	if (retval != 0) {
		/* there was a problem getting the error code */
		printf("error getting socket error code: %s\n", strerror(retval));
		return kNoNetworkClient;
	}

	if (error != 0) {
		/* socket has a non zero error status */
		printf("socket error: %s\n", strerror(error));
		connected = false;
		return kNoNetworkClient;
	}

	return kOk;
}

/*
 * Send UDP datagram to the client with raw data,
 * containing specified data farmat (ID, rawMessage)
 * @param raw data of message
 * @param size of message
 * @return succes
 */
int CNetworkConnection::sendPacket(char* _data, int _size)
{
	int sent;
	if (netCfg.newsockfd >= 0 && connected ) {
		int error = 0;

		if (checkConnection()!=kOk)
			return kNoNetworkClient;

		sent = write(netCfg.newsockfd,_data,_size);
		//sent = send(netCfg.newsockfd, _data, _size, 0);
		if (sent != _size) {
			printf("ERROR writing to socket\n");

			if (error != 0) {
				/* socket has a non zero error status */
				printf("socket error: %s\n", strerror(error));
				connected = false;
				return kNoNetworkClient;
			}
		}
	} else {
		return kNoNetworkClient;
	}

	return kOk;
}

/*
 * Recieves UDP message from network,
 * checks wheter message contains controlling or data information
 * @_data - reference for buffer where raw packet data will be copied, should be size PACKET_SIZE -1
 * @return - type of message ENetMessageType
 */
int CNetworkConnection::recieveMessage(char *_data, size_t _size, bool _waitAll)
{
	//printf("Recieving message.\n");
	//printf("Socket number %d\n",netCfg.newsockfd);

	int recv_ret = -1;
	int recFlag = -1;

	if (netCfg.newsockfd == -1)
		return -1;

	//char * packet = new char[PACKET_SIZE];
	socklen_t addr_size = sizeof(netCfg.serverStorage);

	do {
		if (!connected)
			return -1;

		if (checkConnection()!=kOk) {
			recv_ret = -1;
			connected = false;
			break;
		}

		//printf("reading\n");
		if (_waitAll)
			recv_ret = recv(netCfg.newsockfd,_data,_size,MSG_WAITALL);
		else
			recv_ret = recv(netCfg.newsockfd,_data,_size,0);

		// sleep(1);

	} while (recv_ret < 0);
	//printf("recieved: %d\n",recv_ret);

	if (recv_ret>0) {
		recFlag = _data[0];
	} else if (recFlag ==0) {
		recFlag = -1;
		connected = false;
	} else {
		recFlag = -1;
		connected = false;
	}

	switch (recFlag) {
	case kNetMessageData:
		//memcpy(_data,&_data[1], sizeof(char) * (PACKET_SIZE - 1));
		break;
	case kNetMessageCfg:
		//{ID,flags]
		//memcpy(_data,&_data[1], sizeof(char) * (8));
		break;
	case kNetMessageInform:
		//memcpy(_data, &packet[1], sizeof(char) * (recv_ret - 1));
		break;
	}

	//printf("Recieved message: %d\n",recFlag);

	//delete[] packet;

	return recFlag;
}

/*
 * Get connection state
 * @return bool state
 */

bool CNetworkConnection::isConnected()
{
	return connected;
}

