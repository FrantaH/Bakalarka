/*----------------------------------------------------------------------------*/
/* DESCRIPTION:         TODO
/* AUTHOR:              Petr Frenstatsky
/* URL:                 www.audified.com
/* COPYRIGHT:		2017 AUDIFIED, All Rights Reserved
/*----------------------------------------------------------------------------*/

#include "CNetworkConnection.h"

class IStreamer
{
public:

	virtual int runStreaming() = 0;
	virtual int stopStreaming() = 0;
	virtual int initStreamer(CNetworkConnection *_netConn) = 0;
	virtual void idle() = 0;
	virtual bool isStreaming() = 0;
};

