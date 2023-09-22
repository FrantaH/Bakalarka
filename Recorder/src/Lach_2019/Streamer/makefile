#Martin Lach 2019

SHARED=../Shared
CXX=arm-buildroot-linux-uclibcgnueabihf-c++
CPPFLAGS=-std=c++11 -lboost_system  -lpthread -lboost_thread -g -DALSA_SENDER -I$(SHARED)
LPPDFLAGS=-lpthread -lasound

all: arm_server

arm_server: CNetworkConnection.o main.o server.o connection_manager.o mime_types.o reply.o api_handler.o request_handler.o request_parser.o connection.o CAlsaStreamer.o CStreamerController.o
	mkdir -p ../Build
	$(CXX) -o ../Build/Server $^ $(LPPDFLAGS)  $(CPPFLAGS)

server.o: server.cpp
	$(CXX) $(CPPFLAGS) -c $^
main.o: main.cpp
	$(CXX) $(CPPFLAGS) -c $^
connection_manager.o: connection_manager.cpp
	$(CXX) $(CPPFLAGS) -c $^
mime_types.o: mime_types.cpp
	$(CXX) $(CPPFLAGS) -c $^
request_handler.o: request_handler.cpp
	$(CXX) $(CPPFLAGS) -c $^
api_handler.o: api_handler.cpp
	$(CXX) $(CPPFLAGS) -c $^
request_parser.o: request_parser.cpp
	$(CXX) $(CPPFLAGS) -c $^
connection.o: connection.cpp
	$(CXX) $(CPPFLAGS) -c $^
reply.o: reply.cpp
	$(CXX) $(CPPFLAGS) -c $^
CStreamerController.o:  CStreamerController.cc
	$(CXX) $(CPPFLAGS) -c $^
CAlsaStreamer.o:  CAlsaStreamer.cc
	$(CXX) $(CPPFLAGS) -c $^
CNetworkConnection.o: $(SHARED)/CNetworkConnection.cpp $(SHARED)/CNetworkConnection.h
	$(CXX) $(CPPFLAGS) -c $(SHARED)/CNetworkConnection.cpp -o CNetworkConnection.o
clean:
	$(RM) *.o ../Build/server
