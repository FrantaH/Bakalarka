

$all: ../CNetworkConnection.o  CReciever.o
	mkdir -p ../Build
	g++  -std=c++11 -pthread -g  -o ../Build/Reciever CNetworkConnection.o CReciever.o argvparser-20060124/libargvparser.a
	
CReciever.o: CReciever.cc
	g++  -std=c++11 -pthread -c CReciever.cc
../CNetworkConnection.o: ../Shared/CNetworkConnection.cpp ../Shared/CNetworkConnection.h
	g++ -std=c++11 -pthread -c ../Shared/CNetworkConnection.cpp 
	
argvparser-20060124/libargvparser.a :
	cd argvparser-20060124; make; cd ..;

clean:
	rm -f *.o ../Build/Reciever
