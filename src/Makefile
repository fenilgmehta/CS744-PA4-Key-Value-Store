
CUSTOM_HPPS = MyDebugger.hpp MyMemoryPool.hpp

CLIENT_DEPENDENTS = $(CUSTOM_HPPS) KVMessage.hpp KVClientLibrary.hpp
SERVER_DEPENDENTS = $(CUSTOM_HPPS) KVMessage.hpp KVCache.hpp KVStore.hpp

# -------------------------------------------------------

# REFER: https://stackoverflow.com/questions/1452671/disable-all-gcc-warnings
CXXFLAGS_FINAL=-std=c++17 -pthread -O3 -w

all: final_start KVClient KVServer final_end

final_start:
	@echo "FINAL Build Started...\n"

final_end:
	@echo "\nFINAL Build complete :)"

KVClient: KVClient.cpp $(CLIENT_DEPENDENTS)
	$(CXX) $(CXXFLAGS_FINAL) $< -o $@

KVServer: KVServer.cpp $(SERVER_DEPENDENTS) KVStoreFileNames_FINAL.obj
	$(CXX) $(CXXFLAGS_FINAL) $< KVStoreFileNames_FINAL.obj -o $@

KVStoreFileNames_FINAL.obj: KVStoreFileNames.cpp KVStoreFileNames.h
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

KVServer_db: KVServer.cpp $(SERVER_DEPENDENTS) KVStoreFileNames_DEBUG.obj
	$(CXX) $(CXXFLAGS_FINAL) $< KVStoreFileNames_DEBUG.obj -o KVServer_db

KVStoreFileNames_DEBUG.obj: KVStoreFileNames.cpp KVStoreFileNames.h
	$(CXX) $(CXXFLAGS_FINAL) $< -c -o $@

# -------------------------------------------------------

clean:
	rm -f KVClient KVServer KVClient_db KVServer_db *.o KVStoreFileNames_FINAL.obj KVStoreFileNames_DEBUG.obj
