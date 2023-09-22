// Martin Lach 2019
//

#include <iostream>
#include <string>
#include <stdlib.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include "server.hpp"


#include <pthread.h>
#include <signal.h>


int main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 4)
    {
      std::cerr << "Usage: http_server <address> <port> <doc_root>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    receiver 0.0.0.0 80 .\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    receiver 0::0 80 .\n";
      return 1;
    }

    
int err;
	int mode = kMode_Streaming; //==0

	int port = 7891;

	printf("------------------------------\n--------AUDIO STREAMER--------\n------------------------------\n");

	printf("Number of channels: %d\n",NUM_CHANNELS);
	printf("Sample size: %d\n",SAMPLE_SIZE);
	printf("Packet size: %d\n",PACKET_SIZE);
	printf("Sample rate: %d\n",SAMPLE_RATE);
	printf("Mode: %d\n",  kMode_Streaming);

	//Initialization of stream controller
	CStreamerController *streamerController = new CStreamerController();
	if ((err = streamerController->init(mode))!=kOk) 
		return 3;
		//Run main loop
	http::server::server s(argv[1], argv[2], argv[3], streamerController);
    boost::thread t(boost::bind(&http::server::server::run, &s));
	
		if ((err = streamerController->init2(port))!=kOk) 
		return 3;
	
	std::thread control(&CStreamerController::idle, streamerController);


    control.join();
    t.join();

  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}
