
CUSTOM_HPPS = MyDebugger.hpp MyVector.hpp MyMemoryPool.hpp

CLIENT_DEPENDENTS = $(CUSTOM_HPPS) KVMessage.hpp KVClientLibrary.hpp
SERVER_DEPENDENTS = $(CUSTOM_HPPS) KVMessage.hpp KVCache.hpp KVStore.hpp

# -------------------------------------------------------

# REFER: https://stackoverflow.com/questions/1452671/disable-all-gcc-warnings
CXXFLAGS_FINAL=-std=c++17 -pthread -O2 -w

all: final_start KVClient KVServer final_end
	@echo "\nFINAL Build complete :)"

final_start:
	@echo "FINAL Build Started...\n"

final_end:
	@echo "\nFINAL Build complete :)"

KVClient: KVClient.cpp $(CLIENT_DEPENDENTS)
	$(CXX) $(CXXFLAGS_FINAL) $< -o $@

KVServer: KVServer.cpp $(SERVER_DEPENDENTS) KVStoreFileNames_FINAL.o
	$(CXX) $(CXXFLAGS_FINAL) $< KVStoreFileNames_FINAL.o -o $@

KVStoreFileNames_FINAL.o: KVStoreFileNames.cpp KVStoreFileNames.h
	$(CXX) $(CXXFLAGS_FINAL) $< -c -o $@

# -------------------------------------------------------

debug: debug_start KVServer_db KVClient_db debug_end

debug_start:
	@echo "DEBUG Build Started...\n"
	$(eval CXXFLAGS_FINAL = -std=c++17 -pthread -Wall -Wextra -DDEBUGGING_ON)

debug_end:
	@echo "\nDEBUG Build complete :)"

KVClient_db: KVClient.cpp $(CLIENT_DEPENDENTS)
	$(CXX) $(CXXFLAGS_FINAL) $< -o KVClient_db

KVServer_db: KVServer.cpp $(SERVER_DEPENDENTS) KVStoreFileNames_DEBUG.o
	$(CXX) $(CXXFLAGS_FINAL) $< KVStoreFileNames_DEBUG.o -o KVServer_db

KVStoreFileNames_DEBUG.o: KVStoreFileNames.cpp KVStoreFileNames.h
	$(CXX) $(CXXFLAGS_FINAL) $< -c -o $@

# -------------------------------------------------------

clean:
	rm -f KVClient KVServer *.o
