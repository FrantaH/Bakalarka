
#include <time.h>
#include <cstring>
#include <stdlib.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fstream>
#include <unistd.h>
#include "../Shared/CNetworkConnection.h"
#include "argvparser-20060124/argvparser.h"

const int N_STORED = 100000;  // number of packets which we store at maximum - size of the circular buffer
const int FOUT_BUFF_SIZE = 10;  // number of packets which we store in the output buffer before write out and flush

class CReciever {
public:
    CReciever();
    ~CReciever();

    int init(const char* _IPAddress,const  char* _out_dir_name,const  char* _out_file_name,const  char* _log_file_name,const bool _showlevel);
    void sendConfigurationMessage(uint32_t cfg, uint32_t value);

private:
    static void unpack_packet(char * packet, uint32_t * timestamp, uint32_t * n_samples,
                              uint8_t * n_c, char * buffer);
	static void unpack_inform_packet(char * packet, CReciever *_reciever, uint32_t timestamp);
    //Main processing method
    //static void processRecievedData(CReciever *_reciever);
    static void processRecievedData(CReciever *_reciever);
    static void calcBandwidth(CReciever *_reciever);
    static void recieveData(CReciever *_reciever);
    

    char * files_out [NUM_CHANNELS+2u];
    CNetworkConnection *network;
    char * p_data[N_STORED];  //array of pointers of received data
    uint32_t p_data_timestamp[N_STORED];  //array of pointers of received data timestamps
    std::mutex p_data_empty_mut[N_STORED]; //array of mutexes which makes this thread wait if the right timestamp was not received yet
    std::condition_variable p_data_empty_cv[N_STORED];
    bool p_data_empty[N_STORED];
    int n_samples_stored[N_STORED]; //array storing info about # of samples in each p_data elem
    std::mutex p_data_mutex[N_STORED]; //array of mutexes directing access to p_data
    volatile uint32_t recievedPacket;    //index of recieved packet
    volatile uint32_t currentPacketIndexRec; //current packet timestamp
    int dropRate;   //drop rate of reciever
    std::mutex rateMutex;   //mutex for drop rate calculation
    int actPointerCB;   //hot pointer to circular buffer for writing
    std::mutex actPointerCB_Mutex;   //mutex for hot pointer to circular buffer for writing
    uint32_t cur_write_wait; //number of packets for which we are currently waiting for to write
    bool flush_alert; //informing this thread about whether packet was received or it should just flush the buffers onto the output
    bool show_level; //true if to calculate levels and log them (debug info)
    const char * out_file_name;
    const char * log_file_name;
    const char * out_dir_name;

    //Threads
    std::thread *processThread;
    std::thread *recievingThread;
    std::thread *measurementThread;

    bool processThreadRunning;
    bool recievingThreadRunning;
    bool measurementThreadRunning;

};
