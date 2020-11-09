CXXFLAGS=-O2 -std=c++17 -pthread

all: KVClient KVServer

KVClient: KVClient.cpp
	$(CXX) $< -o $@ $(CXXFLAGS)

KVServer: KVServer.cpp
	$(CXX) $< -o $@ $(CXXFLAGS)

clean:
	rm -f KVClient KVServer
