#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

// ---------------------------------------------------------------------------------------------------------------------

#include "KVCache.h"
#include "KVStore.h"
#include "MyVector.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
MyVectorTemplate(int, _INT)

// ---------------------------------------------------------------------------------------------------------------------

// REFER: https://www.geeksforgeeks.org/enumeration-enum-c/
enum CacheReplacementPolicyType {
    CacheTypeLRU, // Will be implemented for the assignment
    CacheTypeLFU  // Will NOT be implemented for the assignment
};

struct ServerConfig {
    int32_t listening_port;
    int32_t thread_pool_size_initial;
    int32_t thread_pool_growth;
    int32_t clients_per_thread;
    int32_t cache_size;

    // Of NO use as only one Cache Replacement Policy will be implemented for the Assignment
    enum CacheReplacementPolicyType cache_replacement_policy;
} globalServerConfig;

/* Read config file "KVServer.conf" and store the values in "globalServerConfig" */
void read_server_config() {
    static const char CONFIG_FILE[] = "KVServer.conf";

    // TODO: write code to read "CONFIG_FILE" and store the values in "globalServerConfig"
    // Key comes before "=", and Value comes after "="
    // Each Key-Value pair is newline ("\n") separated
}

void server_init() {
    // Initialize the cache,
}

// ---------------------------------------------------------------------------------------------------------------------

/* One instance for each Worker Thread */
struct WorkerThreadInfo {
    pthread_t thread_obj;

    // Worker Thread will iterate through this vector for list of clients to serve using "epoll(...)"
    struct MyVector_INT client_fds;

    // Main Thread will insert new client File Descriptors to this vector and then the Worker Thread
    // will insert all the Client File Descriptors in this into "client_fds" at regular intervals.
    struct MyVector_INT client_fds_new;

    // Lock to ensure only one of Main Thread and Worker thread are working on "client_fds_new"
    pthread_mutex_t mutex_new_client_fds;
} WorkerThreadInfo_DEFAULT;

MyVectorTemplate(struct WorkerThreadInfo, _WorkerThreadInfo)

void *worker_thread(void *ptr) {
    struct WorkerThreadInfo *thread_conf = (struct WorkerThreadInfo *) ptr;
    int32_t i;

    while (1) {
        // TODO - Use epoll and serve clients using Cache

        // TODO - remove client File Descriptors whose connection has closed/ended

        pthread_mutex_lock(&(thread_conf->mutex_new_client_fds));
        if ((thread_conf->client_fds_new).n != 0) {
            // New clients have been assigned to this thread by the Main Thread
            for (i = 0; i < (thread_conf->client_fds_new).n; ++i) {
                mv_push_back_INT(&(thread_conf->client_fds), (thread_conf->client_fds_new).arr[i]);
            }
            mv_clear_INT(&(thread_conf->client_fds_new));
        }
        pthread_mutex_unlock(&(thread_conf->mutex_new_client_fds));

        break;  // TODO: remove this line
    }

    return NULL;  // can modify this to return something else
}

void WorkerThreadInfo_init(struct WorkerThreadInfo *ptr) {
    mv_init_INT(&(ptr->client_fds), globalServerConfig.clients_per_thread);
    mv_init_INT(&(ptr->client_fds_new), globalServerConfig.clients_per_thread);
    pthread_mutex_init(&(ptr->mutex_new_client_fds), NULL);
}

void WorkerThreadInfo_free(struct WorkerThreadInfo *ptr) {
    mv_free_INT(&(ptr->client_fds));
    mv_free_INT(&(ptr->client_fds_new));
    pthread_mutex_destroy(&(ptr->mutex_new_client_fds));
}

void WorkerThreadInfo_start_thread(struct WorkerThreadInfo *ptr) {
    // TODO: write code to start the worker thread with "WorkerThreadInfo" pointer = ptr
}

// ---------------------------------------------------------------------------------------------------------------------

void main_thread() {
    read_server_config();

    // Create "globalServerConfig.thread_pool_size_initial" threads
    struct MyVector_WorkerThreadInfo thread_pool;

    // 4 is multiplied to ensure sufficient space is there in the vector to accommodate for
    // future increase in the number of threads in the "thread_pool"
    mv_init_WorkerThreadInfo(&thread_pool, 4 * globalServerConfig.thread_pool_size_initial);
    for (int i = 0; i < globalServerConfig.thread_pool_size_initial; ++i) {
        WorkerThreadInfo_init(&(thread_pool.arr[i]));
        WorkerThreadInfo_start_thread(&(thread_pool.arr[i]));
    }
    // this will give better performance as compared to calling "push_back_default" multiple times
    thread_pool.n = globalServerConfig.thread_pool_size_initial;

    // TODO: Setup a listening socket on a port specified in the config file
    // TODO: if "globalServerConfig.listening_port" is already in use, then print the error message and exit

    // rr_current_idx - the last "WorkerThreadInfo" index which was used to add the last client request received
    // rr_idx_test - used to store the index of the "WorkerThreadInfo" which is to be tested for assigning the new client
    // rr_iter - is used just like "i" in a for loop only
    int32_t rr_current_idx = -1, rr_idx_test, rr_iter;
    int rr_success;

    int client_fd_new;

    while (1) {
        // TODO
        // Perform accept() on the listening socket and pass each established connection
        // to one of the "thread_pool.n" threads using Round Robin fashion
        client_fd_new = 1;  // TODO: replace "1" with the code to get the client connection File Descriptor

        // Assign the client to one of the threads in the "thread_pool"
        rr_current_idx = (rr_current_idx + 1) % thread_pool.n;
        rr_success = 0;

        for (rr_iter = 0; rr_iter < thread_pool.n; ++rr_iter) {
            rr_idx_test = (rr_current_idx + rr_iter) % thread_pool.n;

            pthread_mutex_t *mutex_to_test = &(thread_pool.arr[rr_idx_test].mutex_new_client_fds);
            pthread_mutex_lock(mutex_to_test);
            if (thread_pool.arr[rr_idx_test].client_fds.n < globalServerConfig.clients_per_thread) {
                // Assign the client connection to the worker thread "thread_pool.arr[rr_idx_test]"
                mv_push_back_INT(&(thread_pool.arr[rr_idx_test].client_fds_new), client_fd_new);

                // unlock the mutex and exit the loop
                pthread_mutex_unlock(mutex_to_test);
                rr_success = 1;
                break;
            }
            pthread_mutex_unlock(mutex_to_test);
        }

        if (rr_success) {
            rr_current_idx = rr_idx_test;
        } else {
            rr_current_idx = thread_pool.n;
            mv_push_back_default_WorkerThreadInfo(&thread_pool);  // Add new "WorkerThreadInfo" object with default values to the vector
            WorkerThreadInfo_init(&(thread_pool.arr[rr_current_idx]));  // Initialise the newly inserted "WorkerThreadInfo" object

            // Insert the new client File Descriptor in "client_fds_new"
            // No need to acquire the lock as the Worker Thread for this object is not running
            mv_push_back_INT(&(thread_pool.arr[rr_current_idx].client_fds_new), client_fd_new);
            WorkerThreadInfo_start_thread(&(thread_pool.arr[rr_current_idx]));  // Start the Worker Thread
        }

        // break;  // TODO: remove this line
    }

    mv_free_WorkerThreadInfo(&thread_pool);
}

// ---------------------------------------------------------------------------------------------------------------------

int main() {
    main_thread();
    return 0;
}

#pragma clang diagnostic pop
