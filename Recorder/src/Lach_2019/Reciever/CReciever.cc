/**
 * edited by Martin Lach
 * 
 * The activity of this script is divided into two threads:
 * - receiving thread - receives packets from the network and stores them
 *                      in circular buffer
 * - processing thread - processes the data from the circular buffer and outputs
 *                       it into the output files
 * The circular buffer is simple array - p_data. The mutual access 
 * to the elements is guarded by array of mutexes p_data_mutex. To make
 * the processing thread wait for incoming packets, another array of mutexes
 * p_data_empty is used - locked when the element of p_data is not filled yet.
 * The processing thread also stored the incoming data into temporary buffers,
 * which are flushed into output files once in a while (flushing after every
 * packet is too time-consuming).
 */


#include <csignal>
#include <cstdint>
#include "CReciever.h"
#include <fstream>

using namespace CommandLineProcessing;

CReciever::CReciever()
{
    network = NULL;
    processThread = NULL;
    recievingThread = NULL;
    measurementThread =NULL;
    processThreadRunning = false;
    recievingThreadRunning = false;
    measurementThreadRunning = false;
    for (int i = 0; i < N_STORED; i++)  {
	  p_data[i] = NULL;
          p_data_timestamp[i] = 0;
	}
    for (uint8_t i = 0; i < (NUM_CHANNELS+2u); i++) {
      files_out[i] = NULL;
    }
}

CReciever::~CReciever()
{
    if(network)
        delete network;
    if(processThread)
    {
        processThreadRunning = false;
        processThread->join();
        delete processThread;
    }
    if(recievingThread)
    {
        recievingThreadRunning = false;
        recievingThread->join();
        delete recievingThread;
	
	for (int i = 0; i < N_STORED; i++)  {
	  if(p_data[i])
	    delete[] p_data[i];
	}
    }
    if(measurementThread)
    {
        measurementThreadRunning = false;
        measurementThread->join();
        delete measurementThread;
    }
}

int CReciever::init(const char* _IPAddress,const char* _out_dir_name,const char* _out_file_name,const char* _log_file_name,const bool _showlevel)
{
    int err;
    network = new CNetworkConnection();
    if((err = network->connectToServer((char*)_IPAddress,port_number)) != kOk)
    {
        std::cout << "cannot connect to server" << std::endl;
        return err;
    }

    show_level = _showlevel;
    out_file_name = _out_file_name;
    out_dir_name = _out_dir_name;
    log_file_name = _log_file_name;
    // --------------- INIT OF THE STRUCTURES FOR STORING INCOMING DATA --------------

    for (int i = 0; i < N_STORED; i++)  {
        p_data[i] = new char[SAMPLES_PER_PACKET * NUM_CHANNELS * SAMPLE_SIZE];
        p_data_timestamp[i] = 0;
//           p_data_empty[i].lock(); //Orig
            p_data_empty[i] = true; //Katka
    }
    rateMutex.unlock();

    for (uint8_t i = 0; i < (NUM_CHANNELS+2u); i++) {
      files_out[i] = new char[2048];
    }


    recievedPacket = 0;
    currentPacketIndexRec = 0;
    dropRate = 0;
    cur_write_wait = -1;
    flush_alert = false;

    // ----------------------- RUNNING THE PROCESSING THREAD ------------------------
    processThreadRunning = true;
    processThread = new std::thread(processRecievedData,this);

    // ---------------------- RUNNING THE RECEIVING THREAD ------------------------
    recievingThreadRunning = true;
    recievingThread = new std::thread(recieveData,this);

    // ---------------------- RUNNING THE THROUGHTPUT MEASUREMENT THREAD ------------------------
    measurementThreadRunning = true;
    measurementThread = new std::thread(calcBandwidth,this);


    
    return kOk;
}

/*
 * Extracts the information from the packet. See README for the format
 * of the packet and the data.
 * @packet - pointer to the content of the packet
 * @timestamp - order number of the packet
 * @n_samples - # of samples stored in the packet
 * @n_c - # of channels (not really used so far 
 *                        as n_channels is fixed in config.h!)
 * @buffer - the audio data from the packet
 */
void CReciever::unpack_packet(char * packet, uint32_t * timestamp, uint32_t * n_samples,
                    uint8_t * n_c, char * buffer) {
    memcpy(timestamp, packet, TIMESTAMP_BYTES);
    memcpy(n_samples, packet+TIMESTAMP_BYTES, NSAMPLE_BYTES);
    memcpy(n_c, packet+TIMESTAMP_BYTES+NSAMPLE_BYTES, NCHANNELS_BYTES);

    memcpy(buffer, packet+TIMESTAMP_BYTES+NSAMPLE_BYTES+NCHANNELS_BYTES, *n_samples * NUM_CHANNELS * SAMPLE_SIZE);
}

void CReciever::unpack_inform_packet(char * packet, CReciever *_reciever, uint32_t timestamp)
{
	uint32_t code = *(uint32_t*)&packet[1];
	FILE * logfile;
	logfile = fopen(_reciever->log_file_name, "a");
	
	if(code == 1)
		fprintf(logfile,"\nfile %s started playing at %d.\n", (char*) packet+5, timestamp);//file started playing
	else if(code == 0)
		fprintf(logfile,"\nfile %s stopped playing at %d.\n", (char*) packet+5, timestamp);//file started playing;//file stopped playing
	else if(code==kRecord){
		fprintf(logfile,"\nStarting recording.\n");
		_reciever->sendConfigurationMessage(kRunControl, 1);//stream
	}
	else if(code==kRecordPlay){
		fprintf(logfile,"\nStarting recording and playing.\n");
		_reciever->sendConfigurationMessage(kRunControl, 2);//stream and play
	}
	else if(code == kExit)
		exit(0);
		
		fclose(logfile);
}
void CReciever::sendConfigurationMessage(uint32_t cfg, uint32_t value)
{

   char packet[PACKET_SIZE];
   memset(packet,0,PACKET_SIZE*sizeof(char));
   packet[0] = kNetMessageCfg;
   *(uint32_t*)(packet+1) = cfg;
   *(uint32_t*)(packet+5) = value;
   
   if(network)
       network->sendPacket(packet,PACKET_SIZE);
}

/*
 * Main processing function
 * Stores data into files in raw format
 * @_reciever pointer to reciever object
 */
void CReciever::processRecievedData(CReciever *_reciever) {
    
    //----------------------- OPEN THE OUTPUT FILES -------------------------
    //---[START]----------------OWN modification--------------[START]------//

    for (uint8_t i_c = 0; i_c < (NUM_CHANNELS); i_c++) {
      snprintf(_reciever->files_out[i_c], 2048, "%s/%s%02d.raw",_reciever->out_dir_name, _reciever->out_file_name, i_c+1u);
    }
    snprintf(_reciever->files_out[NUM_CHANNELS], 2048, "%s/%s%02dd.raw",_reciever->out_dir_name, _reciever->out_file_name, NUM_CHANNELS);
    snprintf(_reciever->files_out[NUM_CHANNELS+1u], 2048, "%s/%s%02ddd.raw",_reciever->out_dir_name, _reciever->out_file_name, NUM_CHANNELS);

    std::ofstream rawfiles[NUM_CHANNELS+2u]; 
    for (uint8_t i_c = 0; i_c < (NUM_CHANNELS+2u); i_c++) {
        rawfiles[i_c].open(_reciever->files_out[i_c], std::ios::out | std::ios::binary);
        if (!rawfiles[i_c].is_open()) {
            std::cout << "ERR: file " << i_c+1 << " " << _reciever->files_out[i_c] << " cannot be opened" << std::endl;
            exit(1);
        }
    }
    //---[END]----------------OWN modification--------------[END]-------//

    // temporary buffers for received data
    // they are emptied after receiving FOUT_BUFF_SIZE packets or 5 seconds without
    // receiving any
    char **incoming_data = new char*[(NUM_CHANNELS + 2u)];
    for(uint8_t i = 0; i< (NUM_CHANNELS + 2u); i++)
    {
      incoming_data[i] = new char [SAMPLES_PER_PACKET*FOUT_BUFF_SIZE*SAMPLE_SIZE];
    }
    uint32_t i_id = 0; // index into incoming data, where we should currently write
    int samplesChannels[NUM_CHANNELS];
    memset(samplesChannels,0x0,NUM_CHANNELS*sizeof(int));

    float last_s = 0;
    float last_ds = 0;
    int gain_i = 0;
    //int max_gain[NUM_CHANNELS+2u];
    float max_gain[NUM_CHANNELS+2u];

    for (uint32_t i = 0; ; i++) {
        if(!_reciever->processThreadRunning)
            break;

//Katka
        _reciever->cur_write_wait = i;
        std::unique_lock<std::mutex> lk(_reciever->p_data_empty_mut[i % N_STORED]);
        _reciever->p_data_empty_cv[i % N_STORED].wait(lk, [&]{return ((_reciever->p_data_empty[i % N_STORED] == false) || (!_reciever->processThreadRunning));});
        if(!_reciever->processThreadRunning) break;
        _reciever->p_data_empty[i % N_STORED] = true;
        lk.unlock();
        _reciever->p_data_empty_cv[i % N_STORED].notify_one();

        // packet we waited for, was received -> store it in incoming data
        _reciever->p_data_mutex[i % N_STORED].lock();

        _reciever->actPointerCB_Mutex.lock();
        _reciever->actPointerCB = i % N_STORED;
        _reciever->actPointerCB_Mutex.unlock();

        for (int i_s = 0; i_s < _reciever->n_samples_stored[i % N_STORED]; i_s++) {
            for (uint8_t i_c = 0; i_c < NUM_CHANNELS; i_c++) {
                for(uint16_t i_p = 0; i_p < SAMPLE_SIZE; i_p++)
                {
                    incoming_data[i_c][i_id+i_p] = *(_reciever->p_data[i % N_STORED] + SAMPLE_SIZE*(i_s*NUM_CHANNELS + i_c)+i_p);
                 }
            }


            i_id += SAMPLE_SIZE;
        }
        //fprintf(stdout, "i_id %d\n", i_id);
        _reciever->p_data_mutex[i % N_STORED].unlock();
	
        if (i_id >= SAMPLES_PER_PACKET*FOUT_BUFF_SIZE*SAMPLE_SIZE) {
          float s1, d, dd;
          uint8_t lastchan = NUM_CHANNELS;
          uint8_t buff[4];
          uint8_t *arr;
            
          for (uint32_t s = 0; s < i_id/SAMPLE_SIZE; s++){
            buff[0]=(uint8_t)incoming_data[lastchan-1][s*4+0];
            buff[1]=(uint8_t)incoming_data[lastchan-1][s*4+1];
            buff[2]=(uint8_t)incoming_data[lastchan-1][s*4+2];
            buff[3]=(uint8_t)incoming_data[lastchan-1][s*4+3];
            s1=*(float*)&buff;
            d = abs(last_s/2 - s1/2);
            dd = abs(last_ds/2 - d/2);
            arr = (uint8_t *)&d;
            incoming_data[lastchan][s*4+0] = arr[0];
            incoming_data[lastchan][s*4+1] = arr[1];
            incoming_data[lastchan][s*4+2] = arr[2];
            incoming_data[lastchan][s*4+3] = arr[3];
            
            arr = (uint8_t *)&dd;
            incoming_data[lastchan+1][s*4+0] = arr[0];
            incoming_data[lastchan+1][s*4+1] = arr[1];
            incoming_data[lastchan+1][s*4+2] = arr[2];
            incoming_data[lastchan+1][s*4+3] = arr[3];
            
            
            last_s = s1;
            last_ds = d;
          }





            //fprintf(stdout, "Entering 1\n");
            //fprintf(stdout, "i_id %d\n", i_id);
            //fprintf(stdout, "gain_i %d\n", gain_i);

	    if ( (_reciever->show_level) && ( gain_i > 30 )) {
              fprintf(stdout, "Gains:");
            }
            for (uint8_t i_c = 0; i_c < (NUM_CHANNELS + 2u); i_c++) {
              //fprintf(stdout, "writing i_c %d\n", i_c);
              //---[START]----------------OWN modification--------------[START]------//
              rawfiles[i_c].write(incoming_data[i_c], i_id);
              if (_reciever->show_level) {
                //fprintf(stdout, "Entering 4\n");
                uint32_t s1u = 0;
                uint8_t buff[4];
                float s1f;
                for (uint16_t s = 0; s < i_id / SAMPLE_SIZE; s++) {
                  //fprintf(stdout, "s: %d, i_id: %d, SS:%d\n", s, i_id, SAMPLE_SIZE);
                  //s1u = (uint32_t) ( ((uint8_t)incoming_data[i_c][s*4+0] << 24) | ((uint8_t)incoming_data[i_c][s*4+1] << 16) | ((uint8_t)incoming_data[i_c][s*4+2] << 8) | ((uint8_t)incoming_data[i_c][s*4+3] << 0));
                  buff[0]=(uint8_t)incoming_data[i_c][s*4+0];
                  buff[1]=(uint8_t)incoming_data[i_c][s*4+1];
                  buff[2]=(uint8_t)incoming_data[i_c][s*4+2];
                  buff[3]=(uint8_t)incoming_data[i_c][s*4+3];
                  s1f=*(float*)&buff;
                  if(abs(s1f) > max_gain[i_c]){
                    max_gain[i_c]=abs(s1f);
                  }
                }
              }

            }
            if(_reciever->show_level){
              //fprintf(stdout, "Entering 2\n");
              gain_i++;
              if(gain_i > 30){
                gain_i=0;
                for (uint8_t i_c = 0; i_c < (NUM_CHANNELS + 2u); i_c++) {
                  //fprintf(stdout, " %2.0f%%", max_gain[i_c] / 83886.07 );
                  fprintf(stdout, " %2.0f%%", max_gain[i_c]*100.0 );
//                  fprintf(stdout, " %8d", max_gain[i_c]);
                  max_gain[i_c]=0.0;
                }
                fprintf(stdout, "\n");
              }
            }
            i_id = 0;
            //fprintf(stdout, "Leaving 1\n");
              
        }
    }
    
    
    for(uint8_t i = 0; i< (NUM_CHANNELS + 2u); i++)
    {
      delete []incoming_data[i];
    }
    delete []incoming_data;
    
    //---[START]----------------OWN modification--------------[START]------//
    for (uint8_t i_c = 0; i_c < (NUM_CHANNELS +2u); i_c++) {
	rawfiles[i_c].flush();
        rawfiles[i_c].close();
    }
    //---[END]----------------OWN modification--------------[END]-------//
}

/*
 * Receives the data from the network and stores it in the circular buffer.
 * @_reciever pointer to reciever object
 */
void CReciever::recieveData(CReciever *_reciever) {



    
    char * buffer = new char[SAMPLES_PER_PACKET * NUM_CHANNELS * 50];
    char * packet = new char[PACKET_SIZE];

    // ------------------------------ RECEIVE THE DATA ------------------------------
    uint32_t timestamp = 0;
    uint32_t n_samples = 0;
    uint8_t n_c = 32;
    uint32_t index = 0;

    int recv_ret;

    std::cout << "recieving thread lets start" << std::endl;
    while (_reciever->recievingThreadRunning) {
        // this either receives the packet from the network and continue after the loop
        // or the timeout for receiving the packet is spent, then the other thread 
        // is alerted to flush its buffers through flush_alert+unlocking and it goes
        // back to waiting for incoming packets
	

        int recFlag;
       // recFlag = _reciever->network->recieveMessage(packet,true);
       while ((recFlag = _reciever->network->recieveMessage(packet, PACKET_SIZE, true)) == -1) {
			;
		}


        if (!_reciever->recievingThreadRunning) break;
        {

            switch(recFlag)
            {
            case kNetMessageData:
                unpack_packet(&packet[1], &timestamp, &n_samples, &n_c, buffer);
                break;
            case kNetMessageInform:
                //printf("Inform message: %s\n",&packet[1]);
                unpack_inform_packet(packet, _reciever, timestamp);
                break;
            case kNetMessageInformRMS:
            {

                continue;
            }
	    default:
                break;
            }
        }
	
        _reciever->rateMutex.lock();
        _reciever->recievedPacket+=PACKET_SIZE;
	
	
	//check droprate
        if(timestamp!=(_reciever->currentPacketIndexRec+1))
	{
          //printf("\nWarr: packed dropped. Last recieved: %d, current packet: %d\n",_reciever->currentPacketIndexRec,timestamp);
          _reciever->dropRate+=(timestamp - _reciever->currentPacketIndexRec -1);
	}
        _reciever->currentPacketIndexRec = timestamp;
        _reciever->rateMutex.unlock();

        _reciever->p_data_mutex[index % N_STORED].lock();
        _reciever->p_data_timestamp[index % N_STORED] = timestamp;
        char * data = _reciever->p_data[index % N_STORED];
        memcpy(data, buffer, SAMPLES_PER_PACKET * NUM_CHANNELS * SAMPLE_SIZE);
	
        _reciever->n_samples_stored[index % N_STORED] = n_samples; // save also #samples
        _reciever->flush_alert = false; // to inform the other thread that we received smth
// Katka
        std::unique_lock<std::mutex> lk(_reciever->p_data_empty_mut[index % N_STORED]);
        _reciever->p_data_empty[index % N_STORED] = false;
        lk.unlock();
        _reciever->p_data_empty_cv[index % N_STORED].notify_one();
// Katka
        //_reciever->p_data_empty[index % N_STORED].unlock(); // wake up the other thread // orig
        _reciever->p_data_mutex[index % N_STORED].unlock();
	index ++;
    }

    delete[] packet;
    delete[] buffer;
}

void CReciever::calcBandwidth(CReciever *_reciever)
{
  
  while(_reciever->measurementThreadRunning)
  {
    _reciever->rateMutex.lock();
    float currBand = _reciever->recievedPacket/1024.f/1024.f;
    _reciever->recievedPacket = 0;
    int currDrop = _reciever->dropRate;
    _reciever->dropRate = 0;
    uint32_t curPacketIndex = _reciever->currentPacketIndexRec;
    _reciever->rateMutex.unlock();

    _reciever->actPointerCB_Mutex.lock();
    int CBpointer = _reciever->actPointerCB;
    _reciever->actPointerCB_Mutex.unlock();

    _reciever->p_data_mutex[CBpointer].lock();
    uint32_t writeDrop = curPacketIndex - _reciever->p_data_timestamp[CBpointer];
    _reciever->p_data_mutex[CBpointer].unlock();

    std::cout << "\rCurrent dataflow: " << currBand << " MB/s Current timestamp: " << _reciever->currentPacketIndexRec<< " Drop rate: " << currDrop <<" Write droped: " << writeDrop << ";" << std::endl;
    fflush(stdout);

    sleep(1);
  }
}

int main(int argc, const char *argv[]) {

    bool show_level = false;
    bool parameter_error = false;
    std::string IP_addr = "127.0.0.1";
    std::string in_gain_file_name = "gains.txt";
    std::string out_dir_name = "./";
    std::string out_file_name = "out_ch";
    std::string log_file_name = "output_log";

    ArgvParser cmd;
    cmd.setIntroductoryDescription("Audio recorder.");
    cmd.setHelpOption("help", "h", "Print this help page");

    cmd.defineOption("IP", "IP of the recording device. (string, required parameter)",
                     ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("IP", "I");


    cmd.defineOption("out-dir", "Directory where are stored output files. (string)",
                     ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("out-dir", "d");
    cmd.defineOption("log-file", "File to store log info. (string)",
                     ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("log-file", "l");

    cmd.defineOption("out-file", "Prefix name of output files followed by [0-9][0-9] as the channel ID. (string)",
                     ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("out-file", "f");

    cmd.defineOption("show-level", "Show level of input channels. For debug only.. can be CPU extensive.",
                     ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("show-level", "s");

    int result = cmd.parse(argc, argv);

    if (result != ArgvParser::NoParserError) {
      std::cout << cmd.parseErrorDescription(result) << std::endl;
      exit(1);
    }

    if (cmd.foundOption("IP")) { IP_addr = cmd.optionValue("IP"); } else {
      std::cout << "Parameter --IP has to be set!" << std::endl;
      parameter_error = true;
    }
   /* if (cmd.foundOption("gain-file")) { in_gain_file_name = cmd.optionValue("gain-file"); } else {
      std::cout << "Parameter --gain-file has to be set!" << std::endl;
      parameter_error = true;
    }*/
    if (cmd.foundOption("out-dir")) { out_dir_name = cmd.optionValue("out-dir"); }
    if (cmd.foundOption("out-file")) { out_file_name = cmd.optionValue("out-file"); }
    if (cmd.foundOption("log-file")) { log_file_name = cmd.optionValue("log-file"); }
    if (cmd.foundOption("show-level")) { 
      show_level=true; 
      if(sizeof(float) != 4){
        fprintf(stdout, "ERROR: The float is not 4 bytes, so it is incompatibile with data sent. Therefor --show-level will show mess. Switching off.\n");
        show_level=false;
      }
    }

    if (parameter_error || cmd.foundOption("print-params")) {
      fprintf(stdout, "Controlls (press): \n\ts - Start recording\n\te - Stop recording\n\tk - Stop recording and finish\n\n");
      fprintf(stdout, "List of values of all parameters:\n");
      fprintf(stdout, "%30s  %s\n", "PARAMETER", "VALUE");
      fprintf(stdout, "%30s  %s\n", "--IP", IP_addr.c_str());
      fprintf(stdout, "%30s  %s\n", "--out-dir", out_dir_name.c_str());
      fprintf(stdout, "%30s  %s\n", "--out-file", out_file_name.c_str());
      fprintf(stdout, "%30s  %s\n", "--log-file", log_file_name.c_str());
      fprintf(stdout, "%30s  %s\n", "--show-level", show_level ? "YES" : "NO");
    }

    if (parameter_error) {
      exit(1);
    }

    fprintf(stdout, "Setting up\n");
    
    	FILE * logfile;
	logfile = fopen(log_file_name.c_str(), "w");
	fclose(logfile);

    CReciever *reciever = new CReciever();
    if(reciever->init(IP_addr.c_str(), out_dir_name.c_str(), out_file_name.c_str(),log_file_name.c_str(), show_level)!=kOk)
      exit(1);
    
    
    bool cont = true;
    char c;
    while(cont)
    {
      c = fgetc(stdin);
      switch (c)
      {
	case 's':
	  fprintf(stdout, "Starting streaming ... \n");
	  reciever->sendConfigurationMessage(kRunControl, 1);
	  break;
	case 'e':
	  fprintf(stdout, "Stopping streaming ... \n");
	  exit(0);
	default:
	  continue;
      }
    }

    delete reciever;

}
