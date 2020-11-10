#include <iostream>
#include <cstdint>
#include <cstring>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <vector>
#include <list>
#include <iterator>
// ---------------------------------------------------------------------------------------------------------------------

#include "MyDebugger.hpp"
#include "MyMemoryPool.hpp"
#include "KVMessage.hpp"
#include "KVCache.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "LocalValueEscapesScope"
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

    int32_t listening_port;  // the port number to bind the socket to
    int32_t socket_listen_n_limit;  // max number of connection the socket should listen to
    int32_t thread_pool_size_initial;  // initial number of threads to be created
    int32_t thread_pool_growth;  // the number of extra threads to be created if all threads have reached the client limits
    int32_t clients_per_thread;  // number of clients that are to be served per thread
    int32_t cache_size;  // number of entries that can be kept in the cache

    // Of NO use as only one Cache Replacement Policy will be implemented for the Assignment
    enum CacheReplacementPolicyType cache_replacement_policy;

    ServerConfig() {
        listening_port = 12345;
        socket_listen_n_limit = 1024;
        thread_pool_size_initial = 2;
        thread_pool_growth = 2;
        clients_per_thread = 5;
        cache_size = 5;
        cache_replacement_policy = CacheTypeLRU;
    }

    /* Read config file "KVServer.conf" and store the values in "globalServerConfig" */
    void read_server_config(const char *configFile = "KVServer.conf") {
        // Read "CONFIG_FILE" and store the values in "globalServerConfig"
        // Key comes before " ", and Value comes after " "
        // Each Key-Value pair is newline ("\n") separated
        std::fstream conf_file;
        conf_file.open(configFile, std::ios::in);
        std::string key;
        int32_t val;

        // LISTENING_PORT 12345
        // SOCKET_LISTEN_N_LIMIT 1024
        // THREAD_POOL_SIZE_INITIAL 2
        // THREAD_POOL_GROWTH 2
        // CLIENTS_PER_THREAD 5
        // CACHE_SIZE 5
        while ((not conf_file.eof()) && conf_file.is_open()) {
            conf_file >> key >> val;
            if (key == "LISTENING_PORT") listening_port = val;
            else if (key == "SOCKET_LISTEN_N_LIMIT") socket_listen_n_limit = val;
            else if (key == "THREAD_POOL_SIZE_INITIAL") thread_pool_size_initial = val;
            else if (key == "THREAD_POOL_GROWTH") thread_pool_growth = val;
            else if (key == "CLIENTS_PER_THREAD") clients_per_thread = val;
            else if (key == "CACHE_SIZE") cache_size = val;
            else log_warning("Invalid server config parameter = \"" + key + "\"");
        }

        conf_file.close();
    }
};

// ---------------------------------------------------------------------------------------------------------------------

/* One instance for each Worker Thread */
struct WorkerThreadInfo {
    pthread_t thread_obj;

    // Worker Thread will iterate through this vector for list of clients to serve using "epoll(...)"
    int32_t client_fds_count;

    // Main Thread will insert new client File Descriptors to this vector and then the Worker Thread
    // will insert all the Client File Descriptors in this into "client_fds" at regular intervals.
    std::vector<int> client_fds_new;

    // Lock to ensure only one of Main Thread and Worker thread are working on "client_fds_new"
    // std::mutex is faster than pthread_mutex_t when tested
    std::mutex mutex_new_client_fds, mutex_serving_clients;

    MemoryPool<KVMessage> *pool_manager;
    KVCache *kv_cache;

    uint32_t thread_id;

    // REFER: https://stackoverflow.com/questions/30867779/correct-pthread-t-initialization-and-handling#:~:text=pthread_t%20is%20a%20C%20type,it%20true%20once%20pthread_create%20succeeds.
    WorkerThreadInfo(int32_t clientsPerThread, MemoryPool<KVMessage> *poolManager, KVCache *kvCache,
                     uint32_t threadId) :
            thread_obj(),
            client_fds_count(0),
            client_fds_new(),
            mutex_new_client_fds(),
            pool_manager{poolManager},
            kv_cache{kvCache},
            thread_id{threadId} {
        client_fds_new.reserve(clientsPerThread);
    }

    void start_thread() {
        // Start the worker thread with "WorkerThreadInfo" pointer = ptr
        pthread_create(&thread_obj, nullptr, worker_thread, reinterpret_cast<void *>(this));
    }

};

struct WorkerThreadInfo WorkerThreadInfo_DEFAULT();

struct ServerConfig *global_server_config;
struct KVCache *globalKVCache;


void *worker_thread(void *ptr) {
    auto thread_conf = static_cast<struct WorkerThreadInfo *>(ptr);
    log_info(std::string("Thread ID = ") + std::to_string(thread_conf->thread_id) + " : started");

    /* Creating epoll instance */
    int epollfd;
    epollfd = epoll_create1(0);
    if (epollfd < -1) {
        log_error("Thread ID = " + std::to_string(thread_conf->thread_id) + " : Failed to create epoll instance...");
        thread_conf->client_fds_count = -1;
        return nullptr;
    }

    /* Creating necessary fd in epoll */
    int event_count;
    struct epoll_event events[global_server_config->clients_per_thread];

    struct KVMessage message;

    while (true) {
        // Use epoll and serve clients using KVCache/KVStore
        // REFER: https://suchprogramming.com/epoll-in-3-easy-steps/
        // REFER: https://stackoverflow.com/questions/46591671/epoll-wait-events-buffer-reset
        // log_info("Calling \"epoll\"");
        event_count = epoll_wait(epollfd, events, global_server_config->clients_per_thread, 3000);

        thread_conf->mutex_serving_clients.lock();
        if (event_count != -1) {
            for (int i = 0; i < event_count; i++) {
                // REFER: https://stackoverflow.com/questions/52976152/tcp-when-is-epollhup-generated
                if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || (!(events[i].events & EPOLLIN))) {
                    log_error(std::string() + "cerr: Epoll event error = \"" + std::to_string(events[i].events) + "\"");
                    // close(events[i].data.fd);
                    break;
                }

                // REFER: https://stackoverflow.com/questions/12340695/how-to-check-if-a-given-file-descriptor-stored-in-a-variable-is-still-valid
                int bytesRead = read(events[i].data.fd, reinterpret_cast<void *>(&message.status_code),
                                     sizeof(uint8_t));

                if (bytesRead == 0) {
                    // Connection was closed
                    // REFER: https://stackoverflow.com/questions/8707601/is-it-necessary-to-deregister-a-socket-from-epoll-before-closing-it
                    // REFER: https://stackoverflow.com/questions/4724137/epoll-wait-receives-socket-closed-twice-read-recv-returns-0
                    log_info("FD closed: " + std::to_string(events[i].data.fd), true);
                    close(events[i].data.fd); // Will unregister the File Descriptor from epoll
                    --(thread_conf->client_fds_count);
                    continue;
                }
                if (not message.is_request_code_valid()) {
                    log_error("Thread ID = " + std::to_string(thread_conf->thread_id)
                              + " : Invalid request code = " + std::to_string(message.status_code));
                    write(events[i].data.fd, reinterpret_cast<const void *>(&KVMessage::StatusCodeValueERROR),
                          sizeof(uint8_t));
                    continue;
                }

                read(events[i].data.fd, reinterpret_cast<void *>(message.key), 256);
                message.calculate_key_hash();  // This was to be done by CACHE, but CACHE is skipped
                if (message.is_request_code_GET()) {
                    // TODO: verify cache
                    // bool res = kvPersistentStore.read_from_db(&message);
                    bool res = thread_conf->kv_cache->cache_GET(&message);
                    write(
                            events[i].data.fd,
                            reinterpret_cast<const void *>(
                                    (res) ? (&KVMessage::StatusCodeValueSUCCESS)
                                          : (&KVMessage::StatusCodeValueERROR)
                            ),
                            sizeof(uint8_t)
                    );
                    write(
                            events[i].data.fd,
                            reinterpret_cast<const void *>(
                                    (res) ? (message.value) : (KVMessage::ERROR_MESSAGE)
                            ),
                            256
                    );
                } else if (message.is_request_code_PUT()) {
                    read(events[i].data.fd, reinterpret_cast<void *>(message.value), 256);
                    // TODO: verify cache
                    // kvPersistentStore.write_to_db(&message);
                    thread_conf->kv_cache->cache_PUT(&message);
                    write(
                            events[i].data.fd,
                            reinterpret_cast<const void *>(&KVMessage::StatusCodeValueSUCCESS),
                            sizeof(uint8_t)
                    );
                } else {
                    // DELETE request code
                    // TODO: verify cache
                    // bool res = kvPersistentStore.delete_from_db(&message);
                    bool res = thread_conf->kv_cache->cache_DELETE(&message);
                    write(
                            events[i].data.fd,
                            reinterpret_cast<const void *>(
                                    (res) ? (&KVMessage::StatusCodeValueSUCCESS)
                                          : (&KVMessage::StatusCodeValueERROR)
                            ),
                            sizeof(uint8_t)
                    );
                    if (not res) {
                        write(
                                events[i].data.fd,
                                reinterpret_cast<const void *>(KVMessage::ERROR_MESSAGE),
                                256
                        );
                    }
                }
            }
        }
        thread_conf->mutex_serving_clients.unlock();

        thread_conf->mutex_new_client_fds.lock();
        if (!(thread_conf->client_fds_new).empty()) {
            // New clients have been assigned to this thread by the Main Thread
            for (uint32_t i = 0; i < (thread_conf->client_fds_new).size(); ++i) {
                events[0].events = EPOLLIN;
                events[0].data.fd = thread_conf->client_fds_new.at(i);
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, thread_conf->client_fds_new.at(i),
                              &events[0])) {
                    log_error("Thread ID = " + std::to_string(thread_conf->thread_id) + " : epoll ctl failed...");
                    return nullptr;
                }
                // thread_conf->client_fds.push_back(thread_conf->client_fds_new.at(i));
                ++(thread_conf->client_fds_count);

            }
            thread_conf->client_fds_new.clear();
        }
        thread_conf->mutex_new_client_fds.unlock();

        // break;  // This is only to be used for testing/debugging
    }

    // return nullptr;  // can modify this to return something else
}

// ---------------------------------------------------------------------------------------------------------------------

std::list<WorkerThreadInfo> *global_thread_pool;

void main_thread() {
    log_info("+ Server initialization started...");

    log_info("    [1/3] Reading server config");
    ServerConfig serverConfig{};
    serverConfig.read_server_config();
    global_server_config = &serverConfig;

    log_info("    [2/3] Initializing memory pool");
    MemoryPool<KVMessage> memPoolKVMessage(true);
    memPoolKVMessage.init(
            serverConfig.thread_pool_size_initial * serverConfig.clients_per_thread,
            2
    );

    log_info("    [3/4] Initializing Persistent Storage (Hard disk) helpers");
    kvPersistentStore.init_kvstore();  // This is present in KVStore.hpp

    log_info("    [4/4] Initializing Cache");
    KVCache kvCache(serverConfig.cache_size);
    globalKVCache = &kvCache;

    log_info("Server initialization finished :)", false, true);

    // ----------------------------------------------------

    // Create "serverConfig.thread_pool_size_initial" threads
    // REFER: https://stackoverflow.com/questions/16465633/how-can-i-use-something-like-stdvectorstdmutex
    std::list<WorkerThreadInfo> thread_pool;
    // thread_pool.reserve(serverConfig.thread_pool_size_initial);
    global_thread_pool = &thread_pool;

    for (int32_t i = 0; i < serverConfig.thread_pool_size_initial; ++i) {
        // WorkerThreadInfo worker(serverConfig.clients_per_thread, &memPoolKVMessage, &kvCache, i + 1);
        // thread_pool.push_back(worker);
        thread_pool.emplace_back(serverConfig.clients_per_thread, &memPoolKVMessage, &kvCache, i + 1);
        thread_pool.rbegin()->start_thread();
    }

    // REFER: https://stackoverflow.com/questions/16486361/creating-a-basic-c-c-tcp-socket-writer
    // Setup a listening socket on a port specified in the config file

    // the most useful file descriptor
    // socket create and verification
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        log_error("SOCET creation failed...");
        log_error("Exiting (status=62)");
        exit(62);
    }

    // assign IP, PORT
    struct sockaddr_in serv_addr{};
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // TODO: verify the use of "htonl" here
    serv_addr.sin_port = htons(serverConfig.listening_port);

    // Binding newly created socket to given IP
    if ((bind(sockfd, reinterpret_cast<struct sockaddr *>(&serv_addr), sizeof(serv_addr))) != 0) {
        log_error("Socket BIND failed... port number already used: " + std::to_string(serverConfig.listening_port));
        log_error("Exiting (status=63)");
        exit(63);
    }
    // Now server is ready to listen
    if ((listen(sockfd, serverConfig.socket_listen_n_limit)) != 0) {
        // MAYBE "serverConfig.listening_port" is already in use
        log_error("Socket LISTEN failed...");
        log_error("Exiting (status=64)");
        exit(64);
    }

    // rr_iter - is used just like "i" in a for loop only
    size_t rr_iter;
    int rr_success;

    struct sockaddr_in client_addr{};
    unsigned int client_len;
    int client_fd_new;

    log_success("Server waiting for clients 😃 on port number = " + std::to_string(serverConfig.listening_port),
                true, true);

    auto listIter = thread_pool.begin();
    while (true) {
        // Perform accept() on the listening socket and pass each established connection
        // to one of the "thread_pool.n" threads using Round Robin fashion
        client_len = sizeof(client_addr);
        client_fd_new = accept(sockfd, reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);
        if (client_fd_new < 0) {
            log_error("Socket failed to ACCEPT client...wait for next client");
            continue;
        }

        // REFER: https://stackoverflow.com/questions/4282369/determining-the-ip-address-of-a-connected-client-on-the-server
        log_info(std::string("---> New Client IP = ") + inet_ntoa(client_addr.sin_addr)
                 + ", Port = " + std::to_string(ntohs(client_addr.sin_port)));

        // Assign the client to one of the threads in the "thread_pool"
        rr_success = 0;

        for (rr_iter = 0; rr_iter < thread_pool.size(); ++rr_iter, ++listIter) {
            if (listIter == thread_pool.end()) listIter = thread_pool.begin();

            std::mutex &mutex_to_test = listIter->mutex_new_client_fds;
            mutex_to_test.lock();
            if (listIter->client_fds_count < serverConfig.clients_per_thread) {
                // Assign the client connection to the worker thread "thread_pool.arr[rr_idx_test]"
                listIter->client_fds_new.push_back(client_fd_new);

                // unlock the mutex and exit the loop
                mutex_to_test.unlock();
                rr_success = 1;
                break;
            }
            mutex_to_test.unlock();
        }

        if (rr_success) {
            log_info("Main Thread: client assigned to thread_id = " + std::to_string(listIter->thread_id));
            ++listIter;
        } else {
            // Add new "WorkerThreadInfo" object with default values to the vector
            // Initialise the newly inserted "WorkerThreadInfo" object
            // WorkerThreadInfo worker(serverConfig.clients_per_thread, &memPoolKVMessage, &kvCache, thread_pool.size() + 1);
            // thread_pool.push_back(worker);
            thread_pool.emplace_back(serverConfig.clients_per_thread, &memPoolKVMessage, &kvCache,
                                     thread_pool.size() + 1);

            // Insert the new client File Descriptor in "client_fds_new"
            // No need to acquire the lock as the Worker Thread for this object is not running
            auto templistIter = thread_pool.rbegin();
            templistIter->client_fds_new.push_back(client_fd_new);
            templistIter->start_thread();  // Start the Worker Thread
            listIter = thread_pool.begin();

            for (int32_t i = 1; i < serverConfig.thread_pool_growth; ++i) {
                thread_pool.emplace_back(serverConfig.clients_per_thread, &memPoolKVMessage, &kvCache,
                                         thread_pool.size() + 1);
            }
        }

        // break;  // This is only to be used for testing/debugging
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void signal_callback_handler(int signalNumber) {
    log_warning("CTRL+C pressed. Closing the server...", true);
    for (auto &i : *global_thread_pool) {
        // acquire mutex for all threads so that they stop after serving the clients
        // for that particular iteration
        log_info("Waiting for thread_id = " + std::to_string(i.thread_id) + " / " +
                 std::to_string(global_thread_pool->size()));
        i.mutex_serving_clients.lock();
    }

    if (globalKVCache != nullptr) {
        log_info("Performing cache cleanup");
        globalKVCache->cache_clean();
        log_success("Cache cleaning complete :)", true);
    }

    log_success("Server cleanup complete :)", true, true);
    exit(0);
}

// ---------------------------------------------------------------------------------------------------------------------


int main() {
    // REFER: https://www.tutorialspoint.com/how-do-i-catch-a-ctrlplusc-event-in-cplusplus
    signal(SIGINT, signal_callback_handler);

    main_thread();
    return 0;
}

#pragma clang diagnostic pop
