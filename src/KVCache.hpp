#ifndef PA_4_KEY_VALUE_STORE_KVCACHE_HPP
#define PA_4_KEY_VALUE_STORE_KVCACHE_HPP

#include <shared_mutex>
#include <atomic>

#include "MyDebugger.hpp"
#include "MyVector.hpp"
#include "KVMessage.hpp"
#include "KVStore.hpp"
/*

### C++ function

REFER: StackOverFlow - Hash Function for String
	- https://stackoverflow.com/questions/7666509/hash-function-for-string
REFER: CP-Algorithms
	- https://cp-algorithms.com/string/string-hashing.html
REFER: Code Project
	- https://www.codeproject.com/Articles/716530/Fastest-Hash-Function-for-Table-Lookups-in-C
REFER: Everything about unordered_map
	-https://codeforces.com/blog/entry/21853
REFER: STL - Shared Mutex
	- https://en.cppreference.com/w/cpp/thread/shared_mutex#:~:text=Within%20one%20thread%2C%20only%20one,writing%20at%20the%20same%20time.

REFER: How to make a multiple-read/single-write lock from more basic synchronization primitives?
	- https://stackoverflow.com/questions/27860685/how-to-make-a-multiple-read-single-write-lock-from-more-basic-synchronization-pr


### Conceptual part

REFER: LRU cache design
	- https://stackoverflow.com/questions/2504178/lru-cache-design
REFER: How would you design a “multithreaded” LRU cache using C++ (unordered_map and Linkedlist)?
	- https://softwareengineering.stackexchange.com/questions/376818/how-would-you-design-a-multithreaded-lru-cache-using-c-unordered-map-and-li
REFER: Thread Safe LRU
	- https://github.com/tstarling/thread-safe-lru/tree/master/thread-safe-lru
REFER: High-Throughput, Thread-Safe, LRU Caching
	- https://tech.ebayinc.com/engineering/high-throughput-thread-safe-lru-caching/
REFER: Thread Safe LRU Cache
	- https://codereview.stackexchange.com/questions/193664/thread-safe-lru-cache

### Libraries for reference
REFER: cachelib
	- https://github.com/ksholla20/cachelib/tree/V1.0
*/

#define HASH_TABLE_LEN 16383

struct CacheNodeCircularQueuePtr {
    // REFER:
    //     https://cppstdx.readthedocs.io/en/latest/shared_mutex.html
    //     https://en.cppreference.com/w/cpp/thread/shared_mutex
    //     http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3427.html
    //     https://cppstdx.readthedocs.io/en/latest/shared_mutex.html#_CPPv211shared_lock
    std::shared_mutex rw_lock;
    struct CacheNode *head, *tail;

    CacheNodeCircularQueuePtr() : rw_lock(), head{nullptr}, tail{nullptr} {}
};

struct CacheNode {
    struct KVMessage *message;
    struct CacheNode *left, *right;
    struct CacheNode *prev, *next;
    bool dirtyBit;  // if true, then it needs to be written back to the Persistent Storage

    /* Release memory which was allocated to "KVMessage" either by the "KVServer" or "KVCache" */
    void free_node() {
        free(message);
    }
};

/*
 * • It is assumed that KVMessage pointer has proper values for both Key and Value
 * • Cache size is at-least 𝟭𝟬𝟮𝟰 otherwise, there is performance loss
 *
 * */
struct KVCache {
    uint64_t n;
    MyVector<CacheNodeCircularQueuePtr> hashTable, evictionTable;
    std::atomic_uint64_t evictionIdx;  // REFER: https://stackoverflow.com/questions/31978324/what-exactly-is-stdatomic

    KVCache(uint64_t cache_size) :
            n{cache_size},
            hashTable(16383),
            evictionTable((cache_size > 10240) ? 128 : ((1024 <= cache_size && cache_size < 10240) ? 32 : 1)),
            evictionIdx(0) {
        // TODO - implement the constructor
    }

    struct KVMessage *cache_GET(struct KVMessage *ptr) {
        ptr->calculate_key_hash();

        // TODO
        // dirty bit remain UNCHANGED
    }

    void cache_PUT(struct KVMessage *ptr) {
        ptr->calculate_key_hash();

        // TODO
        // if cache is full, remove 5 % entries and save them to storage

        // SET dirty bit to true
    }

    struct KVMessage *cache_DELETE(struct KVMessage *ptr) {
        ptr->calculate_key_hash();

        // TODO
        // Write to Persistent storage if dirty bit of a CacheNode is true
    }

private:
    CacheNodeCircularQueuePtr get_next_eviction_queue() {
        // --- REFER: https://stackoverflow.com/questions/33554255/c-thread-safe-increment-with-modulo-without-mutex-using-stdatomic
        // --- REFER: https://stackoverflow.com/questions/31978324/what-exactly-is-stdatomic

        // --- TIME = ~1.9 seconds (for Threads=100, Loop=100000)
        // --- EXPLANATION: The function will work just like below 4 lines
        // std::mutex m;
        // m.lock();
        // global = (global + 1) % MOD_VAL;
        // m.unlock();

        // --- IMPORTANT
        // std::atomic_uint32_t global(0);

        // How to make this operation atomic?
        uint64_t index = evictionIdx++;
        uint64_t id = index % evictionTable.n;

        // Move ahead if only a single element is present
        while (evictionTable.at(id).head == evictionTable.at(id).tail) {
            index = evictionIdx++;
            id = index % evictionTable.n;
        }

        // If size could wrap, then re-write the modulo value.
        // oldValue keeps getting re-read.
        // modulo occurs when nothing else updates it.
        uint64_t oldValue = evictionIdx;
        uint64_t newValue = oldValue % evictionTable.n;

        // --- TIME = ~1.2 seconds (for Threads=100, Loop=100000)
        while (!evictionIdx.compare_exchange_weak(oldValue, newValue, std::memory_order_relaxed))
            newValue = oldValue % evictionTable.n;

        // return id;
        return evictionTable[id];
    }

};

#endif // PA_4_KEY_VALUE_STORE_KVCACHE_HPP
