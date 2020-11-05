#include <iostream>
#include <cstdint>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

// ---------------------------------------------------------------------------------------------------------------------

#include "MyDebugger.hpp"
#include "MyVector.hpp"
#include "KVCache.hpp"
#include "MyMemoryPool.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

// ---------------------------------------------------------------------------------------------------------------------
void *worker_thread(void *);

struct ServerConfig {
    // REFER: https://www.geeksforgeeks.org/enumeration-enum-c/
    enum CacheReplacementPolicyType {
        CacheTypeLRU, // Will be implemented for the assignment
        CacheTypeLFU  // Will NOT be implemented for the assignment
    };

    int32_t listening_port;
    int32_t thread_pool_size_initial;
    int32_t thread_pool_growth;
    int32_t clients_per_thread;
    int32_t cache_size;

    // Of NO use as only one Cache Replacement Policy will be implemented for the Assignment
    enum CacheReplacementPolicyType cache_replacement_policy;

    ServerConfig() = default;

    /* Read config file "KVServer.conf" and store the values in "globalServerConfig" */
    void read_server_config(const char *configFile = "KVServer.conf") {
        // TODO: write code to read "CONFIG_FILE" and store the values in "globalServerConfig"
        // Key comes before "=", and Value comes after "="
        // Each Key-Value pair is newline ("\n") separated
    }
};

// ---------------------------------------------------------------------------------------------------------------------

/* One instance for each Worker Thread */
struct WorkerThreadInfo {
    pthread_t thread_obj;

    // Worker Thread will iterate through this vector for list of clients to serve using "epoll(...)"
    struct MyVector<int> client_fds;

    // Main Thread will insert new client File Descriptors to this vector and then the Worker Thread
    // will insert all the Client File Descriptors in this into "client_fds" at regular intervals.
    struct MyVector<int> client_fds_new;

    // Lock to ensure only one of Main Thread and Worker thread are working on "client_fds_new"
    // std::mutex is faster than pthread_mutex_t when tested
    std::mutex mutex_new_client_fds;

    MemoryPool<KVMessage> *pool_manager;
    KVCache *kv_cache;

    // REFER: https://stackoverflow.com/questions/30867779/correct-pthread-t-initialization-and-handling#:~:text=pthread_t%20is%20a%20C%20type,it%20true%20once%20pthread_create%20succeeds.
    explicit WorkerThreadInfo(int32_t clientsPerThread, MemoryPool<KVMessage> *poolManager, KVCache *kvCache) :
            thread_obj(),
            client_fds(clientsPerThread),
            client_fds_new(clientsPerThread),
            mutex_new_client_fds(),
            pool_manager{poolManager},
            kv_cache{kvCache} {}

    void start_thread() {
		// TODO: write code to start the worker thread with "WorkerThreadInfo" pointer = ptr
		pthread_create(&thread_obj, NULL, worker_thread, (void*)this);
    }

};

struct WorkerThreadInfo WorkerThreadInfo_DEFAULT();


void *worker_thread(void *ptr) {
    auto thread_conf = static_cast<struct WorkerThreadInfo *>(ptr);

    int32_t i;

    while (true) {
        // TODO - Use epoll and serve clients using Cache

        // TODO - remove client File Descriptors whose connection has closed/ended

        thread_conf->mutex_new_client_fds.lock();
        if ((thread_conf->client_fds_new).n != 0) {
            // New clients have been assigned to this thread by the Main Thread
            for (i = 0; i < (thread_conf->client_fds_new).n; ++i) {
                thread_conf->client_fds.push_back(thread_conf->client_fds_new.at(i));
            }
            thread_conf->client_fds_new.clear();
        }
        thread_conf->mutex_new_client_fds.unlock();

        break;  // TODO: remove this line
    }

    return nullptr;  // can modify this to return something else
}

// ---------------------------------------------------------------------------------------------------------------------

void main_thread() {
    log_info("+ Server initialization started...");

    log_info("    [1/3] Reading server config");
    ServerConfig globalServerConfig{};
    globalServerConfig.read_server_config();

    log_info("    [2/3] Initializing memory pool");
    MemoryPool<KVMessage> globalPoolKVMessage;
    globalPoolKVMessage.init(
            globalServerConfig.thread_pool_size_initial * globalServerConfig.clients_per_thread
    );

    log_info("    [3/4] Initializing Persistent Storage (Hard disk) helpers");
    kvPersistentStore.init_kvstore();  // This is present in KVStore.hpp

    log_info("    [4/4] Initializing Cache");
    KVCache globalKVCache(globalServerConfig.cache_size);

    log_info("Server initialization finished :)", false, true);

    // ----------------------------------------------------

    // Create "globalServerConfig.thread_pool_size_initial" threads
    // 4 is multiplied to ensure sufficient space is there in the vector to accommodate for
    // future increase in the number of threads in the "thread_pool"
    struct MyVector<WorkerThreadInfo> thread_pool(4 * globalServerConfig.thread_pool_size_initial);

    // this will give better performance as compared to calling "push_back_default" multiple times
    thread_pool.n = globalServerConfig.thread_pool_size_initial;

    for (int i = 0; i < globalServerConfig.thread_pool_size_initial; ++i) {
        thread_pool.push_back(
                WorkerThreadInfo(globalServerConfig.clients_per_thread, &globalPoolKVMessage, &globalKVCache)
        );
        thread_pool.at(i).start_thread();
    }

    // TODO: Setup a listening socket on a port specified in the config file

    int sockfd;
    struct sockaddr_in servaddr, cli_addr;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
    	std::cerr << "Socket creation failed...\n";
        exit(0);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(globalServerConfig.listening_port);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
    	std::cerr << "Socket bind failed...\n";
        exit(0);
    }
    // Now server is ready to listen and verification
    if ((listen(sockfd, 1024)) != 0) {
    	std::cerr << "Listen failed...\n";
        exit(0);
    }
    // TODO: if "globalServerConfig.listening_port" is already in use, then print the error message and exit
    // REFER: https://stackoverflow.com/questions/16486361/creating-a-basic-c-c-tcp-socket-writer

    // rr_current_idx - the last "WorkerThreadInfo" index which was used to add the last client request received
    // rr_idx_test - used to store the index of the "WorkerThreadInfo" which is to be tested for assigning the new client
    // rr_iter - is used just like "i" in a for loop only
    int32_t rr_current_idx = -1, rr_idx_test, rr_iter;
    int rr_success;

    int client_fd_new;
    unsigned int clilen;

    log_success("Server initialization complete :)");

    while (true) {
        // TODO
        // Perform accept() on the listening socket and pass each established connection
        // to one of the "thread_pool.n" threads using Round Robin fashion
        //client_fd_new = 1;  // TODO: replace "1" with the code to get the client connection File Descriptor
    	clilen = sizeof(cli_addr);
    	client_fd_new = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
    	if(client_fd_new < 0){
    		std::cerr << "Failed to accept client...";
    	}
        // Assign the client to one of the threads in the "thread_pool"
        rr_current_idx = (rr_current_idx + 1) % thread_pool.n;
        rr_success = 0;

        for (rr_iter = 0; rr_iter < thread_pool.n; ++rr_iter) {
            rr_idx_test = (rr_current_idx + rr_iter) % thread_pool.n;

            std::mutex &mutex_to_test = thread_pool.arr[rr_idx_test].mutex_new_client_fds;
            mutex_to_test.lock();
            if (thread_pool.arr[rr_idx_test].client_fds.n < globalServerConfig.clients_per_thread) {
                // Assign the client connection to the worker thread "thread_pool.arr[rr_idx_test]"
                thread_pool.at(rr_idx_test).client_fds_new.push_back(client_fd_new);

                // unlock the mutex and exit the loop
                mutex_to_test.unlock();
                rr_success = 1;
                break;
            }
            mutex_to_test.unlock();
        }

        if (rr_success) {
            rr_current_idx = rr_idx_test;
        } else {
            rr_current_idx = thread_pool.n;

            // Add new "WorkerThreadInfo" object with default values to the vector
            // Initialise the newly inserted "WorkerThreadInfo" object
            thread_pool.push_back();

            // Insert the new client File Descriptor in "client_fds_new"
            // No need to acquire the lock as the Worker Thread for this object is not running
            thread_pool.at(rr_current_idx).client_fds_new.push_back(client_fd_new);
            thread_pool.at(rr_current_idx).start_thread();  // Start the Worker Thread
        }

        // break;  // TODO: remove this line
    }
}

// ---------------------------------------------------------------------------------------------------------------------

// TODO: write code to handle CTRL+C signal and write all the cache data to Persistent Storage

// Call cacheClear()

// ---------------------------------------------------------------------------------------------------------------------


int main() {
    main_thread();
    return 0;
}

#pragma clang diagnostic pop
