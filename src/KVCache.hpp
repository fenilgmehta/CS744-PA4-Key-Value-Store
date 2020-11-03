#ifndef PA_4_KEY_VALUE_STORE_KVCACHE_HPP
#define PA_4_KEY_VALUE_STORE_KVCACHE_HPP

#include "MyDebugger.hpp"
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

#define HASH_TABLE_LEN


struct KVCache {
    
};

#endif // PA_4_KEY_VALUE_STORE_KVCACHE_HPP
