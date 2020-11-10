#ifndef PA_4_KEY_VALUE_STORE_KVCACHE_HPP
#define PA_4_KEY_VALUE_STORE_KVCACHE_HPP

#include <shared_mutex>
#include <atomic>
#include <vector>

#include "MyDebugger.hpp"
#include "MyMemoryPool.hpp"
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

struct CacheNode {
    enum EnumDirtyBit {
        DirtyBit_ALLGOOD = 0,
        DirtyBit_DIRTY = 1,
        DirtyBit_NOT_IN_CACHE = 2,
        DirtyBit_TODELETE = 3
    };

    struct KVMessage message;

    // Doubly Linked List (NOT circular)
    struct CacheNode *l1_left, *l1_right;  // Doubly Linked List - layer 1 of Cache
    struct CacheNode *l2_prev, *l2_next;  // Doubly Linked List - layer 2 to maintain LRU info

    int32_t lru_idx;
    int dirty_bit;
    // if 0, the latest value is already present in the Persistent Storage
    // if 1, it needs to be written back/updated to the Persistent Storage
    // if 2, this CacheNode has been invalidated by someone  // MOSTLY this is not required as it would be put back in to Memory Pool
    // if 3, delete this entry from Persistent Storage as well when removing it from cache

    CacheNode() : message(), l1_left{nullptr}, l1_right{nullptr}, l2_prev{nullptr}, l2_next{nullptr},
                  lru_idx{0}, dirty_bit{2} {}

    void set_all(KVMessage *message1,
                 CacheNode *l1Left, CacheNode *l1Right,
                 CacheNode *l2Prev, CacheNode *l2Next,
                 int32_t lruIdx,
                 int dirtyBit) {
        message.hash1 = message1->hash1;
        message.hash2 = message1->hash2;
        message.set_key_fast(message1->key);
        message.set_value_fast(message1->value);
        l1_left = l1Left;
        l1_right = l1Right;
        l2_prev = l2Prev;
        l2_next = l2Next;
        lru_idx = lruIdx;
        dirty_bit = dirtyBit;
    }


    [[nodiscard]] inline bool is_cache_node_presentInCache() const {
        return dirty_bit != EnumDirtyBit::DirtyBit_NOT_IN_CACHE;
    }

    [[nodiscard]] inline bool is_cache_node_allgood() const {
        return dirty_bit == EnumDirtyBit::DirtyBit_ALLGOOD;
    }

    [[nodiscard]] inline bool is_cache_node_dirty() const {
        return dirty_bit == EnumDirtyBit::DirtyBit_DIRTY;
    }

    [[nodiscard]] inline bool is_cache_node_notInCache() const {
        return dirty_bit == EnumDirtyBit::DirtyBit_NOT_IN_CACHE;
    }

    [[nodiscard]] inline bool is_cache_node_deleted() const {
        return dirty_bit == EnumDirtyBit::DirtyBit_TODELETE;
    }
};

struct CacheNodeQueuePtr {
    // REFER:
    //     https://cppstdx.readthedocs.io/en/latest/shared_mutex.html
    //     https://en.cppreference.com/w/cpp/thread/shared_mutex
    //     http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3427.html
    //     https://cppstdx.readthedocs.io/en/latest/shared_mutex.html#_CPPv211shared_lock
    std::shared_mutex rw_lock;
    struct CacheNode *head, *tail;

    CacheNodeQueuePtr() : rw_lock(), head{nullptr}, tail{nullptr} {}
};

/*
 * • It is assumed that KVMessage pointer has proper values for both Key and Value
 * • Cache size is at-least 𝟭𝟬𝟮𝟰 otherwise, there is performance loss
 *
 *  TODO in future
 *      - If cache is full, remove 5 % entries and save them to Persistent Storage
 *      - In the initial implementation, only 1 entry will be removed if cache is full
 *
 * */
struct KVCache {
#define CACHE_TABLE_LEN 16384
    uint64_t nMax;

    // NOTE: CacheNodeQueuePtr->tail will NOT be used in hastTable
    std::vector<CacheNodeQueuePtr> hashTable, lruEvictionTable;
    MemoryPool<CacheNode> cacheNodeMemoryPool;

    // REFER: https://stackoverflow.com/questions/31978324/what-exactly-is-stdatomic
    std::atomic_uint64_t lruTableInsertIdx, lruEvictionIdx;

    explicit KVCache(uint64_t cache_size) :
            nMax{cache_size},
            hashTable(CACHE_TABLE_LEN),  // = 16384
            lruEvictionTable((cache_size >= 10240) ? 128 : ((10240 > cache_size && cache_size >= 1024) ? 32 : 1)),
            cacheNodeMemoryPool(true),
            lruTableInsertIdx(0),
            lruEvictionIdx(0) {
        // TODO - verify if anything more is required - implement the constructor
        cacheNodeMemoryPool.init(cache_size + CACHE_TABLE_LEN, 2);
    }

    /* ASSUMED: ptr->key is correctly filled in ptr
     *
     * IMPORTANT: Will calculate hash1 and hash2 in this method
     *          : Dirty Bit remain UNCHANGED
     *
     * Returns: true if GET was successful (i.e. Key was either present in Cache or Persistent Storage)
     *              - The "Value" corresponding to "ptr->key" will be stored in "ptr->value"
     *        : false if "Key" is not present
     * */

    CacheNode *cache_GET_ptr(struct KVMessage *ptr) {
        ptr->calculate_key_hash();

        uint64_t hashTableIdx = (ptr->hash1) % CACHE_TABLE_LEN;
        std::shared_lock reader_lock(hashTable.at(hashTableIdx).rw_lock);

        // Search through the cache
        // a. entry found - return the value in ptr->value
        // b. entry not found - search Persistent storage and do eviction if the cache is full

        struct CacheNode *cacheNodeIter = hashTable.at(hashTableIdx).head;

        while (cacheNodeIter != nullptr) {
            if (not entry_equals(&(cacheNodeIter->message), ptr)) {
                cacheNodeIter = cacheNodeIter->l1_right;
                continue;
            }

            // MATCH FOUND :)
            log_info("cache_GET_ptr(...) --> Cache HIT");

            if (not cacheNodeIter->is_cache_node_deleted()) {
                std::copy(cacheNodeIter->message.value, cacheNodeIter->message.value + 256, ptr->value);
            }

            // Update the LRU list
            std::unique_lock write_lock(lruEvictionTable.at(cacheNodeIter->lru_idx).rw_lock);

            // IMPORTANT: this is same as the one in "cache_PUT"
            // Update the LRU list
            move_to_head_LRU(&lruEvictionTable.at(cacheNodeIter->lru_idx), cacheNodeIter);

            if (cacheNodeIter->is_cache_node_deleted()) return nullptr;
            return cacheNodeIter;
        }


        log_info("cache_GET_ptr(...) --> Cache MISS");
        if (kvPersistentStore.read_from_db(ptr)) {
            // Get the Key-Value pair in Cache
            reader_lock.unlock();
            return cache_PUT_new_entry(ptr, hashTableIdx);
        }

        return nullptr;  // "Key" neither found in cache nor in persistent storage
    }

    bool cache_GET(struct KVMessage *ptr) {
        auto res = cache_GET_ptr(ptr);
        return res != nullptr;
    }

    CacheNode *cache_PUT_new_entry(struct KVMessage *ptr, uint64_t hashTableIdx) {
        // IMPORTANT ACTION
        CacheNode *new_cacheNode;

        if (is_full())
            new_cacheNode = cache_eviction();
        else
            new_cacheNode = cacheNodeMemoryPool.acquire_instance();

        // mostly there is no possibility of creating any problem
        uint64_t lru_insert_idx = get_next_lru_queue_idx();
        new_cacheNode->set_all(
                ptr,
                nullptr, nullptr,
                nullptr, nullptr,
                lru_insert_idx, CacheNode::DirtyBit_DIRTY
        );

        std::unique_lock write_lock1(hashTable.at(hashTableIdx).rw_lock);
        std::unique_lock write_lock2(lruEvictionTable.at(lru_insert_idx).rw_lock);

        insert_to_head_HT(&hashTable.at(hashTableIdx), new_cacheNode);
        insert_to_head_LRU(&lruEvictionTable.at(lru_insert_idx), new_cacheNode);
        return new_cacheNode;
    }

    /* ASSUMED: ptr->key and ptr->value are correctly filled in ptr
     * IMPORTANT: will calculate hash1 and hash2 here
     * */
    void cache_PUT(struct KVMessage *ptr) {
        ptr->calculate_key_hash();

        uint64_t hashTableIdx = (ptr->hash1) % CACHE_TABLE_LEN;

        // Search through the cache
        // a. entry found - then update the value in ptr->value and update the dirty bit
        // b. entry not found - do eviction if the cache is full and set the dirty bit
        std::shared_lock reader_lock(hashTable.at(hashTableIdx).rw_lock);

        struct CacheNode *cacheNodeIter = hashTable.at(hashTableIdx).head;

        while (cacheNodeIter != nullptr) {
            if (not entry_equals(&(cacheNodeIter->message), ptr)) {
                cacheNodeIter = cacheNodeIter->l1_right;
                continue;
            }

            // MATCH FOUND, we just update the "Value"
            log_info("cache_PUT(...) --> Cache HIT");

            reader_lock.unlock();
            std::unique_lock writer_lock1(hashTable.at(hashTableIdx).rw_lock);
            std::unique_lock writer_lock2(lruEvictionTable.at(cacheNodeIter->lru_idx).rw_lock);

            if (not(cacheNodeIter->is_cache_node_presentInCache())) {
                // The Key has either been deleted or has been evicted from the cache
                // between the unlocking of reader lock and acquiring the writer lock
                // There is a possibility that this CacheNode has been reused for some other cache entry. Hence
                // we acquire a new instance or do cache eviction and work on the new CacheNode object
                if (is_full())
                    cacheNodeIter = cache_eviction();
                else
                    cacheNodeIter = cacheNodeMemoryPool.acquire_instance();

                cacheNodeIter->message.hash1 = ptr->hash1;
                cacheNodeIter->message.hash2 = ptr->hash2;
                cacheNodeIter->message.set_key_fast(ptr->key);

                // It is just an assumption that if the entry was not present in Cache,
                // then we just assume that a new value is being assigned for the key
                cacheNodeIter->dirty_bit = CacheNode::DirtyBit_DIRTY;
            } else if (std::equal(ptr->value, ptr->value + 256, cacheNodeIter->message.value)) {
                // NO change in the dirty_bit as the new and old values match
                if (cacheNodeIter->is_cache_node_deleted()) cacheNodeIter->dirty_bit = CacheNode::DirtyBit_DIRTY;
            } else {
                cacheNodeIter->dirty_bit = CacheNode::DirtyBit_DIRTY;
            }

            cacheNodeIter->message.set_value_fast(ptr->value);

            // IMPORTANT: this is same as the one in "cache_GET"
            // Update the LRU list
            move_to_head_LRU(&lruEvictionTable.at(cacheNodeIter->lru_idx), cacheNodeIter);

            return;
        }
        reader_lock.unlock();

        // As Key is not present in cache, just insert it in cache
        // IMPORTANT: It shall be written back to the storage on eviction or cleaning
        //            We set the Dirty Bit to appropriate value

        // Get the Key-Value pair in Cache

        log_info("cache_PUT(...) --> Cache MISS");
        cache_PUT_new_entry(ptr, hashTableIdx);
    }

    /* ASSUMED: ptr->key is correctly filled where all places after the first occurrence of '\0' have '\0'
     * IMPORTANT: will calculate hash1 and hash2 here
     * */
    bool cache_DELETE(struct KVMessage *ptr) {
        ptr->calculate_key_hash();
        log_info(std::string() + "cache_DELETE(...) --> "
                 + std::to_string(ptr->hash1) + "," + std::to_string(ptr->hash2)
                 + "," + ptr->key + "," + ptr->value);
        uint64_t hashTableIdx = (ptr->hash1) % CACHE_TABLE_LEN;

        // Search through the cache
        // a. entry found - then update the value in ptr->value and update the dirty bit
        // b. entry not found - do eviction if the cache is full and set the dirty bit
        std::shared_lock reader_lock(hashTable.at(hashTableIdx).rw_lock);

        struct CacheNode *cacheNodeIter = hashTable.at(hashTableIdx).head;

        while (cacheNodeIter != nullptr) {
            if (not entry_equals(&(cacheNodeIter->message), ptr)) {
                cacheNodeIter = cacheNodeIter->l1_right;
                continue;
            }

            // MATCH FOUND :)
            log_info("cache_DELETE(...) --> Cache HIT");

            if (cacheNodeIter->is_cache_node_deleted()) {
                log_info("    cache_DELETE(...) --> node already deleted");
                return false;  // The Key has already been deleted
            } else if (cacheNodeIter->is_cache_node_notInCache()) {
                // This is un-expected because we are holding the reader lock.
                // The question arises, how did someone modify the CacheNode ?

                log_error("    cache_DELETE(...) --> UN-expected "
                          "\"cacheNodeIter->is_cache_node_notInCache()\" after match found");

                // Bring it back in cache
                // CacheNode *new_cacheNodeIter = cache_GET_ptr(ptr);
                // if (new_cacheNodeIter == nullptr)
                //     return false;  // entry has been deleted from the storage as well
                //
                // reader_lock.unlock();
                // std::unique_lock writer_lock1(hashTable.at(hashTableIdx).rw_lock);
                // new_cacheNodeIter->dirty_bit = CacheNode::EnumDirtyBit::DirtyBit_TODELETE;
            } else {
                log_info("    cache_DELETE(...) --> performing fast deletion");
                reader_lock.unlock();

                std::unique_lock writer_lock1(hashTable.at(hashTableIdx).rw_lock);
                std::unique_lock writer_lock2(lruEvictionTable.at(cacheNodeIter->lru_idx).rw_lock);
                if (cacheNodeIter->is_cache_node_deleted()) {
                    return false;
                }
                if (cacheNodeIter->is_cache_node_notInCache()) {
                    log_error("    cache_DELETE(...) --> cacheNodeIter->is_cache_node_notInCache()");
                    break;
                }
                cacheNodeIter->dirty_bit = CacheNode::EnumDirtyBit::DirtyBit_TODELETE;
                move_to_head_LRU(&lruEvictionTable.at(cacheNodeIter->lru_idx), cacheNodeIter);
            }
            return true;
        }

        // NO MATCH FOUND
        return kvPersistentStore.delete_from_db(ptr);

        // *** ALTERNATIVE ***

        // // Bring it back in cache
        // CacheNode *new_cacheNodeIter = cache_GET_ptr(ptr);
        // if (new_cacheNodeIter == nullptr)
        //     return false;  // entry has been deleted from the storage as well
        //
        // reader_lock.unlock();
        // std::unique_lock writer_lock1(hashTable.at(hashTableIdx).rw_lock);
        // new_cacheNodeIter->dirty_bit = CacheNode::EnumDirtyBit::DirtyBit_TODELETE;
        // return true;
    }

    /* ASSUMPTION: cache_eviction() will only be called when the cache is full
     * RETURNS: NULL if the KVCache is empty, otherwise CacheNode* of the evicted CacheNode for reuse */
    CacheNode *cache_eviction() {
        static uint64_t evictionCallCount = 0;
        log_info("cache_eviction() called count = " + std::to_string(++evictionCallCount));

        // Write to Persistent storage if dirty bit of a CacheNode is true
        uint64_t eqIdx = get_next_eviction_queue_idx();

        uint64_t i = 0;
        for (; i < CACHE_TABLE_LEN && lruEvictionTable.at(eqIdx).head == nullptr; ++i) {
            eqIdx = get_next_eviction_queue_idx();
        }
        if (i == CACHE_TABLE_LEN && lruEvictionTable.at(eqIdx).head == nullptr) {
            log_warning("cache_eviction(): cache is empty :)");
            return nullptr;
        }

        // find Hash Table Queue Index
        uint64_t hqIdx = lruEvictionTable.at(eqIdx).tail->message.hash1 % CACHE_TABLE_LEN;
        std::unique_lock writer_lock1(hashTable.at(hqIdx).rw_lock);
        std::unique_lock writer_lock2(lruEvictionTable.at(eqIdx).rw_lock);

        CacheNode *ptrToRemove = lruEvictionTable.at(eqIdx).tail;
        if (ptrToRemove->is_cache_node_notInCache()) {
            log_error("ptrToRemove->is_cache_node_notInCache(), trying recursive methodology");

            writer_lock1.unlock();
            writer_lock2.unlock();
            return cache_eviction();
        }

        remove_from_dll_HT(&hashTable.at(hqIdx), ptrToRemove);
        remove_from_dll_LRU(&lruEvictionTable.at(eqIdx), ptrToRemove);

        if (ptrToRemove->is_cache_node_deleted()) {
            kvPersistentStore.delete_from_db(&(ptrToRemove->message));
        } else if (ptrToRemove->is_cache_node_dirty()) {
            kvPersistentStore.write_to_db(&(ptrToRemove->message));
        } else {
            // the updated value is already present in the Persistent Storage
        }

        writer_lock1.unlock();
        writer_lock2.unlock();
        return ptrToRemove;
    }

    void cache_eviction_simple() {
        // Write to Persistent storage if dirty bit of a CacheNode is true
        for (int i = 0; i < 10; ++i) {
            uint64_t eqIdx = get_next_eviction_queue_idx();
            if (lruEvictionTable.at(eqIdx).head == nullptr) continue;

            // find Hash Table Queue Index
            uint64_t hqIdx = lruEvictionTable.at(eqIdx).tail->message.hash1 % CACHE_TABLE_LEN;
            std::unique_lock writer_lock1(hashTable.at(hqIdx).rw_lock);
            std::unique_lock writer_lock2(lruEvictionTable.at(eqIdx).rw_lock);

            CacheNode *ptrToRemove = lruEvictionTable.at(eqIdx).tail;
            if (ptrToRemove->is_cache_node_notInCache()) {
                writer_lock1.unlock();
                writer_lock2.unlock();
                continue;
            }

            remove_from_dll_HT(&hashTable.at(hqIdx), ptrToRemove);
            remove_from_dll_LRU(&lruEvictionTable.at(eqIdx), ptrToRemove);

            if (ptrToRemove->is_cache_node_deleted()) {
                kvPersistentStore.delete_from_db(&(ptrToRemove->message));
            } else if (ptrToRemove->is_cache_node_dirty()) {
                kvPersistentStore.write_to_db(&(ptrToRemove->message));
            } else {
                // the updated value is already present in the Persistent Storage
            }
            cacheNodeMemoryPool.release_instance(ptrToRemove);

            writer_lock1.unlock();
            writer_lock2.unlock();
        }
    }

    /* ASSUMPTION: this method will only be called when closing the KVServer
     * Write all cached data to Persistent Storage */
    void cache_clean() {
        // TODO: Mostly will just have to call "cache_eviction" for all cache entries
        // while (not is_empty()) {
        //     cache_eviction();
        // }

        // for (int32_t i = 0; i < this->nMax; ++i) {
        //     cache_eviction();
        //     cache_eviction();
        //     cache_eviction();
        // }

        while(cache_eviction());
        return;

        CacheNode *ptr;
        for (int32_t i = 0; i < CACHE_TABLE_LEN; ++i) {
            ptr = hashTable.at(i).head;
            while (ptr != nullptr) {
                log_info("    Cache Node evicted = "
                         + std::to_string(ptr->message.hash1) + "," + std::to_string(ptr->message.hash2) + ","
                         + ptr->message.key + "," + ptr->message.value);

                if (ptr->is_cache_node_deleted()) {
                    kvPersistentStore.delete_from_db(&(ptr->message));
                } else if (ptr->is_cache_node_dirty()) {
                    kvPersistentStore.write_to_db(&(ptr->message));
                } else {
                    // the updated value is already present in the Persistent Storage
                    // OR, is_cache_node_notInCache
                }
                ptr = ptr->l1_right;
            }
        }
    }

private:
    [[nodiscard]] inline bool is_not_full() const {
        return (
                       (cacheNodeMemoryPool.memoryBlockPointers.size() * cacheNodeMemoryPool.blockSize) -
                       cacheNodeMemoryPool.n
               ) < nMax;
        // return n < nMax;
    }

    [[nodiscard]] inline bool is_full() const {
        return cacheNodeMemoryPool.n == 0;
    }

    [[nodiscard]] inline bool is_empty() const {
        return (cacheNodeMemoryPool.memoryBlockPointers.size() * cacheNodeMemoryPool.blockSize) ==
               cacheNodeMemoryPool.pool.size();
    }

    uint64_t get_next_eviction_queue_idx() {
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
        uint64_t index = lruEvictionIdx++;
        uint64_t id = index % lruEvictionTable.size();

        // // Move ahead if only a single element is present
        // while (lruEvictionTable.at(id).head == lruEvictionTable.at(id).tail) {
        //     index = lruEvictionIdx++;
        //     id = index % lruEvictionTable.size();
        // }

        // If size could wrap, then re-write the modulo value.
        // oldValue keeps getting re-read.
        // modulo occurs when nothing else updates it.
        uint64_t oldValue = lruEvictionIdx;
        uint64_t newValue = oldValue % lruEvictionTable.size();

        // --- TIME = ~1.2 seconds (for Threads=100, Loop=100000)
        while (!lruEvictionIdx.compare_exchange_weak(oldValue, newValue, std::memory_order_relaxed))
            newValue = oldValue % lruEvictionTable.size();

        return id;
        // return lruEvictionTable.at(id);
    }

    uint64_t get_next_lru_queue_idx() {
        // How to make this operation atomic?
        uint64_t index = lruTableInsertIdx++;
        uint64_t id = index % lruEvictionTable.size();

        // // Move ahead if only a single element is present
        // while (lruEvictionTable.at(id).head == lruEvictionTable.at(id).tail) {
        //     index = lruTableInsertIdx++;
        //     id = index % lruEvictionTable.size();
        // }

        // If size could wrap, then re-write the modulo value.
        // oldValue keeps getting re-read.
        // modulo occurs when nothing else updates it.
        uint64_t oldValue = lruTableInsertIdx;
        uint64_t newValue = oldValue % lruEvictionTable.size();

        // --- TIME = ~1.2 seconds (for Threads=100, Loop=100000)
        while (!lruTableInsertIdx.compare_exchange_weak(oldValue, newValue, std::memory_order_relaxed))
            newValue = oldValue % lruEvictionTable.size();

        return id;
        // return lruEvictionTable.at(id);
    }

    /* ASSUMPTION: ptr is already present in the list pointed by ptrQueue */
    static void move_to_head_LRU(CacheNodeQueuePtr *ptrQueue, CacheNode *ptr) {
        if (ptrQueue->head == ptr) {
            return;  // ptr is already the ptrQueue->head
        }
        remove_from_dll_LRU(ptrQueue, ptr);
        insert_to_head_LRU(ptrQueue, ptr);
    }

    static void remove_from_dll_LRU(CacheNodeQueuePtr *ptrQueue, CacheNode *ptr) {
        // remove the node from NON-Circular Doubly Linked List
        if (ptr->l2_prev == nullptr) {
            // first node in the list
            ptrQueue->head = ptr->l2_next;
        } else {
            ptr->l2_prev->l2_next = ptr->l2_next;
        }

        if (ptr->l2_next == nullptr) {
            // case where ptr is the last node
            ptrQueue->tail = ptr->l2_prev;
        } else {
            // if ptr is not the last node
            ptr->l2_next->l2_prev = ptr->l2_prev;
        }
    }

    /* ASSUMED: "ptrQueue" is a NON-Circular Doubly Linked List */
    static void insert_to_head_LRU(CacheNodeQueuePtr *ptrQueue, CacheNode *ptr) {
        ptr->l2_prev = nullptr;
        ptr->l2_next = ptrQueue->head;

        if (ptrQueue->head == nullptr) {
            // list is empty
            ptrQueue->tail = ptr;
        } else {
            // list is NON empty
            ptrQueue->head->l2_prev = ptr;
        }
        ptrQueue->head = ptr;
    }

    static void remove_from_dll_HT(CacheNodeQueuePtr *ptrQueue, CacheNode *ptr) {
        // remove the node from NON-Circular Doubly Linked List
        if (ptr->l1_left == nullptr) {
            // first node in the list
            ptrQueue->head = ptr->l1_right;
        } else {
            ptr->l1_left->l1_right = ptr->l1_right;
        }

        if (ptr->l1_right == nullptr) {
            // case where ptr is the last node
            ptrQueue->tail = ptr->l1_left;
        } else {
            // if ptr is not the last node
            ptr->l1_right->l1_left = ptr->l1_left;
        }
    }

    /* ASSUMED: "ptrQueue" is a NON-Circular Doubly Linked List */
    static void insert_to_head_HT(CacheNodeQueuePtr *ptrQueue, CacheNode *ptr) {
        ptr->l1_left = nullptr;
        ptr->l1_right = ptrQueue->head;

        if (ptrQueue->head == nullptr) {
            // As mentioned earlier, CacheNodeQueuePtr->tail is NOT used for Layer 1 Doubly Linked List
            // i.e. TAIL is not used in list of "hashTable"
            // list is empty
            ptrQueue->tail = ptr;
        } else {
            // list is NON empty
            ptrQueue->head->l1_left = ptr;
        }
        ptrQueue->head = ptr;
    }

    static bool entry_equals(KVMessage *a, KVMessage *b) {
        return (a->hash1 == b->hash1)
               && (a->hash2 == b->hash2)
               && (std::equal(a->key, a->key + 256, b->key));
    }

#undef CACHE_TABLE_LEN
};

#endif // PA_4_KEY_VALUE_STORE_KVCACHE_HPP
