//
// api_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Martin Lach

#include "api_handler.hpp"
#include <string>
#include <boost/lexical_cast.hpp>
#include "reply.hpp"
#include <iostream>
#include <fstream>
#include "../Shared/AudioStreamer_definitions.h"


const char PHANTOM = 0;
const char GAIN = 1;


namespace http {
namespace server {


void api_handler::handle_request(std::string req, reply& rep, CStreamerController * streamer)
{
	 
	std::string reply_str;
	rep.status = reply::ok;

	
	if(req.find("getlevels")!=std::string::npos)
	{
		//std::cout<<"getting levels...\""<<req<<"\""<<std::endl<<std::fflush;
		reply_str.append(api_handler::get_levels(streamer));
	}
	else if(req.find("gain")!=std::string::npos)
	{
		//std::cout<<"should i change gain?\""<<req<<"\""<<std::endl<<std::fflush;
		api_handler::resolve_set(req, GAIN);
	}
	else if(req.find("phantom")!=std::string::npos)
	{
		//std::cout<<"should i change phantom?\""<<req<<"\""<<std::endl<<std::fflush;
		api_handler::resolve_set(req, PHANTOM);
	}	
	else if(req.find("getplaylistfiles")!=std::string::npos)
	{
		reply_str.append(api_handler::get_playlist_files());
		//std::cout<<"getting levels...\""<<req<<"\""<<api_handler::get_audio_files()<<std::endl<<std::fflush;
		//std::cout<<"should i get all?\""<<req<<"\""<<std::endl<<std::fflush;
	}
	else if(req.find("getaudiofiles")!=std::string::npos)
	{
		reply_str.append(api_handler::get_audio_files());
		//std::cout<<"getting levels...\""<<req<<"\""<<api_handler::get_audio_files()<<std::endl<<std::fflush;
		//std::cout<<"should i get all?\""<<req<<"\""<<std::endl<<std::fflush;
	}
	else if(req.find("getplaylist")!=std::string::npos)
	{
		reply_str.append(api_handler::get_playlist(req));
		//std::cout<<"getting levels...\""<<req<<"\""<<api_handler::get_audio_files()<<std::endl<<std::fflush;
		//std::cout<<"should i get all?\""<<req<<"\""<<std::endl<<std::fflush;
	}
	else if(req.find("save")!=std::string::npos)
	{
		reply_str.append(api_handler::save_playlist(req));
		//std::cout<<"getting levels...\""<<req<<"\""<<api_handler::get_audio_files()<<std::endl<<std::fflush;
		//std::cout<<"should i get all?\""<<req<<"\""<<std::endl<<std::fflush;
	}
	else if(req.find("getall")!=std::string::npos)
	{
		reply_str.append(api_handler::get_all());
		//std::cout<<"should i get all?\""<<req<<"\""<<std::endl<<std::fflush;
	}
	else if(req.find("playrecord")!=std::string::npos)
	{
		reply_str.append(api_handler::save_playlist(req));
		printf("streamer State = %d \n", streamer->getState());
		if (streamer->getState()) 
		{
			char text[FILENAME_LEN] = "turning on playing streaming";
			streamer->sendMessage(kRecordPlay, text);
		}
		//std::cout<<"should i get all?\""<<req<<"\""<<std::endl<<std::fflush;
	}
	else if(req.find("record")!=std::string::npos)
	{
		//reply_str.append(api_handler::save_playlist(req));
		printf("streamer State = %d \n", streamer->getState());
		if (streamer->getState()) 
		{
			char text[FILENAME_LEN] = "turning on streaming";
			streamer->sendMessage(kRecord, text);
		}
		//std::cout<<"should i get all?\""<<req<<"\""<<std::endl<<std::fflush;
	}
	else if(req.find("stop")!=std::string::npos)
	{
		//reply_str.append(api_handler::save_playlist(req));
		printf("streamer State = %d \n", streamer->getState());
		if (streamer->getState()) 
		{
			char text[FILENAME_LEN] = "turning off streaming";
			streamer->sendMessage(kExit, text);
		}
		//std::cout<<"should i get all?\""<<req<<"\""<<std::endl<<std::fflush;
	}
	else
	{
		rep = reply::stock_reply(reply::not_found);
		return;
	}
	
	rep.content.append(reply_str.c_str(), static_cast<int>(reply_str.size()));
	rep.headers.resize(2);
	rep.headers[0].name = "Content-Length";
	rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
	rep.headers[1].name = "Content-Type";
	rep.headers[1].value = "application/json";
}

void api_handler::resolve_set(std::string req, int type){
	int first = req.find('?')+1;
	int last = req.find('=');
	int index, value;
	try{
		index = std::stoi(req.substr(first,last-first));
		value = std::stoi(req.substr(last+1, 100));
	}
	catch (const std::invalid_argument& ia){
		std::cerr << "Invalid argument";
		return;
	}
	if (type==PHANTOM)
		set_phantom(index, value);
	else if (type==GAIN)
		set_gain(index, value);		
}


std::string api_handler::save_playlist(std::string req){
	char name[FILENAME_LEN];
	strcpy(name, "files/playlists/");
	size_t len = 0;
	std::string reply_str = "{\"playlistsongs\":[";
	bool not_empty = false;
	int first = req.find('?')+1;
	int last = req.find('=');
	std::string index, value;
	try{
		index = req.substr(first,last-first);
		value = req.substr(last+1, 10000);
	}
	catch (const std::invalid_argument& ia){
		std::cerr << "Invalid argument";
		return 0;
	}
	FILE *playlistFile;
	strcat(name,index.c_str()); 
	//std::cerr << "Invalid argument"<< name;
	playlistFile = fopen(name,"w");
	if (playlistFile == NULL)
		return 0;	

	fprintf(playlistFile,"%s", value.c_str());
	fclose(playlistFile);

	reply_str.append("]}");
			
	//printf("PEAK: %f\n", RMS[i]);
	
	return reply_str;
		
}
std::string api_handler::get_playlist(std::string req){
	
	char*name=NULL;	
	char Fname[FILENAME_LEN];
	char songs[1024];
	strcpy(Fname, "files/playlists/");
	size_t len = 0;
	std::string reply_str = "{\"playlistsongs\":[";
	bool not_empty = false;
	int first = req.find('?')+1;
	//int last = req.find('=');
	std::string index;
	try{
		index = req.substr(first,1000);
		//value = req.substr(last+1, 100);
	}
	catch (const std::invalid_argument& ia){
		std::cerr << "Invalid argument";
		return 0;
	}
	
	strcat(Fname,index.c_str()); 
	std::ifstream playlistFile;
	playlistFile.open(Fname);
	if (playlistFile.is_open())
	{
		std::string str;
		while(std::getline(playlistFile, str, ','))
		{
		
			reply_str.append("\"");
			reply_str.append(str); 
			reply_str.append("\",");
			not_empty = true;
		}
		if(not_empty)
				reply_str.pop_back();
	}
	playlistFile.close();
		reply_str.append("]}");
				
		//printf("PEAK: %f\n", RMS[i]);
		
		return reply_str;
		
}

void api_handler::set_phantom(int index, int value){
	char filename[FILENAME_LEN];
	sprintf(filename, "/proc/asound/SHARC/phantom#%d",index);
	FILE *pFile;
	std::cout<<"Changing phantom \""<<index<<"\" to \""<<value<<"\""<<std::endl<<std::fflush;
	pFile = fopen(filename,"w");
	if (pFile != NULL)
	{
		fprintf(pFile,"%d", value);
		fclose(pFile);
	}
	
}
void api_handler::set_gain(int index, int value){
	char filename[FILENAME_LEN];
	sprintf(filename, "/proc/asound/SHARC/gain#%d",index);
	FILE *gFile;
	std::cout<<"Changing gain \""<<index<<"\" to \""<<value<<"\""<<std::endl<<std::fflush;
	gFile = fopen(filename,"w");
	if (gFile != NULL)
	{
		fprintf(gFile,"%d", value);
		fclose(gFile);
	}
	
}

std::string api_handler::get_levels(CStreamerController * streamer){
		float* RMS = streamer->RMS;
		std::string reply_str = "{\"levels\":[";
		for (int i = 0; i<NUM_CHANNELS; i++)
		{
			reply_str.append(std::to_string(RMS[i]));
			RMS[i] = 0.f;
			if(i<(NUM_CHANNELS-1))
				reply_str.append(",");
			else
				reply_str.append("],");
				
		//printf("PEAK: %f\n", RMS[i]);
		}
		reply_str.append("\"streaming\":");
		reply_str.append(std::to_string(streamer->isStreaming()));
		reply_str.append(",\"state\":");
		reply_str.append(std::to_string(streamer->getState()));
		reply_str.append(",\"playing\":\"");
		reply_str.append(streamer->playingFile);
		reply_str.append("\",\"connected\":");
		reply_str.append(std::to_string(streamer->isConnected()));
		reply_str.append("}");
		return reply_str;
	}
std::string api_handler::get_audio_files(){
		std::string reply_str = "{\"files\":[";
		bool not_empty = false;
		struct dirent *de;  // Pointer for directory entry 
  
		// opendir() returns a pointer of DIR type.  
		DIR *dr = opendir("files/audio"); 

		if (dr != NULL)  // opendir returns NULL if couldn't open directory 
		{ 
			// for readdir() 
			while ((de = readdir(dr)) != NULL) 
			{
				if(de->d_name != NULL){
				std::string str(de->d_name);
					if(str.find(".wav")!=std::string::npos)
					{
						reply_str.append("\"");
						reply_str.append(str.substr(0, str.size()-4)); 
						
						reply_str.append("\",");
						not_empty = true;
					}
				}
			}
		
			closedir(dr);  
			if(not_empty)
				reply_str.pop_back();
		}   

		reply_str.append("]}");
				
		//printf("PEAK: %f\n", RMS[i]);
		
		return reply_str;
	}
std::string api_handler::get_playlist_files(){
		std::string reply_str = "{\"playlists\":[";
		bool not_empty = false;
		struct dirent *d;  // Pointer for directory entry 
  
		// opendir() returns a pointer of DIR type.  
		DIR *dir = opendir("files/playlists"); 

		if (dir != NULL)  // opendir returns NULL if couldn't open directory 
		{ 
			// for readdir() 
			while ((d = readdir(dir)) != NULL) 
			{
				if(d->d_name != NULL){
					std::string str(d->d_name);
					if(str.find(".")!=0)
					{
						reply_str.append("\"");
						reply_str.append(str); 
						reply_str.append("\",");
						not_empty = true;
					}
				}
			}
		
			closedir(dir);     
			if(not_empty)
				reply_str.pop_back();
		}
		reply_str.append("]}");
				
		//printf("PEAK: %f\n", RMS[i]);
		
		return reply_str;
	}
std::string api_handler::get_all(){
	char filename[FILENAME_LEN];
	int gain;
	FILE *File;
	std::string reply_str = "{\"channels\": [";
	reply_str.append(std::to_string(NUM_CHANNELS));
	reply_str.append("],");
	
	
	reply_str.append("\"gains\":[");
	for (int i = 0; i<NUM_CHANNELS; i++)
	{
		sprintf(filename, "/proc/asound/SHARC/gain#%d",i);
		File = fopen(filename,"r");
		fscanf(File, "%d", &gain);
		if (File != NULL)
		{
			reply_str.append(std::to_string(gain));
			fclose(File);
		}
		if(i<(NUM_CHANNELS-1))
			reply_str.append(",");
		else
			reply_str.append("],");
			

	}
	reply_str.append( "\"phantoms\":[");
	for (int i = 0; i<16; i++)
	{
		sprintf(filename, "/proc/asound/SHARC/phantom#%d",i);
		File = fopen(filename,"r");
		fscanf(File, "%d", &gain);
		if (File != NULL)
		{
			reply_str.append(std::to_string(gain));
			fclose(File);
		}
		if(i<15)
			reply_str.append(",");
		else
			reply_str.append("]}");
	}
	return reply_str;
}

} // namespace server
} // namespace http

