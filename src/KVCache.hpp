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
        DirtyBit_INVALID = 2,
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

    [[nodiscard]] inline bool is_cache_node_dirty() const {
        return dirty_bit == EnumDirtyBit::DirtyBit_DIRTY;
    }

    [[nodiscard]] inline bool is_cache_node_valid() const {
        return dirty_bit == EnumDirtyBit::DirtyBit_ALLGOOD || dirty_bit == EnumDirtyBit::DirtyBit_DIRTY;
    }

    [[nodiscard]] inline bool is_cache_node_deleted() const {
        return dirty_bit == EnumDirtyBit::DirtyBit_TODELETE;
    }

    [[nodiscard]] inline bool is_cache_node_evicted() const {
        return dirty_bit == EnumDirtyBit::DirtyBit_INVALID;
    }

    // TODO: I think this should be removed because Memory Pool is present with the KVServer.cpp
    /* Release memory which was allocated to "KVMessage" either by the "KVServer" or "KVCache" */
    void free_node() {

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
    uint64_t nMax;

    // NOTE: CacheNodeQueuePtr->tail will NOT be used in hastTable
    std::vector<CacheNodeQueuePtr> hashTable, lruEvictionTable;
    MemoryPool<CacheNode> cacheNodeMemoryPool;

    // REFER: https://stackoverflow.com/questions/31978324/what-exactly-is-stdatomic
    std::atomic_uint64_t lruTableInsertIdx, lruEvictionIdx;

    explicit KVCache(uint64_t cache_size) :
            nMax{cache_size},
            hashTable(HASH_TABLE_LEN),  // = 16384
            lruEvictionTable((cache_size >= 10240) ? 128 : ((10240 > cache_size && cache_size >= 1024) ? 32 : 1)),
            cacheNodeMemoryPool(true),
            lruTableInsertIdx(0),
            lruEvictionIdx(0) {
        // TODO - verify if anything more is required - implement the constructor
        cacheNodeMemoryPool.init(cache_size + HASH_TABLE_LEN, 2);
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

        uint64_t hashTableIdx = (ptr->hash1) % HASH_TABLE_LEN;
        std::shared_lock reader_lock(hashTable.at(hashTableIdx).rw_lock);

        if (not is_empty()) {
            // Search through the cache
            // a. entry found - return the value in ptr->value
            // b. entry not found - search Persistent storage and do eviction if the cache is full

            struct CacheNode *cacheNodeIter = hashTable.at(hashTableIdx).head;

            while (cacheNodeIter != nullptr) {
                if (not entry_equals(&(cacheNodeIter->message), ptr)) {
                    cacheNodeIter = cacheNodeIter->l1_right;
                    continue;
                }

                // match found :)
                std::copy(cacheNodeIter->message.value, cacheNodeIter->message.value + 256, ptr->value);

                // Update the LRU list
                std::unique_lock write_lock(lruEvictionTable.at(cacheNodeIter->lru_idx).rw_lock);

                // IMPORTANT: this is same as the one in "cache_PUT"
                // No point of performing any work if cacheNodeIter is already the first node in the LRU Queue
                if (cacheNodeIter->l2_prev != nullptr) {
                    cacheNodeIter->l2_prev->l2_next = cacheNodeIter->l2_next;
                    if (cacheNodeIter->l2_next == nullptr)  // i.e. this is last node in LRU Queue
                        lruEvictionTable.at(cacheNodeIter->lru_idx).tail = cacheNodeIter->l2_prev;
                    else
                        cacheNodeIter->l2_next->l2_prev = cacheNodeIter->l2_prev;

                    cacheNodeIter->l2_prev = nullptr;
                    cacheNodeIter->l2_next = lruEvictionTable.at(cacheNodeIter->lru_idx).head;
                    lruEvictionTable.at(cacheNodeIter->lru_idx).head = cacheNodeIter;
                }

                return cacheNodeIter;
            }
        }


        if (kvPersistentStore.read_from_db(ptr)) {
            // Get the Key-Value pair in Cache
            reader_lock.unlock();

            // IMPORTANT ACTION
            if (is_full())
                cache_eviction();
            CacheNode *new_cacheNode = cacheNodeMemoryPool.acquire_instance();

            uint64_t lru_insert_idx = get_next_lru_queue_idx();
            new_cacheNode->set_all(
                    ptr,
                    nullptr, nullptr, nullptr, nullptr,
                    lru_insert_idx, CacheNode::EnumDirtyBit::DirtyBit_ALLGOOD
            );

            std::unique_lock write_lock1(hashTable.at(hashTableIdx).rw_lock);
            std::unique_lock write_lock2(lruEvictionTable.at(lru_insert_idx).rw_lock);

            new_cacheNode->l1_right = hashTable.at(hashTableIdx).head;
            hashTable.at(hashTableIdx).head = new_cacheNode;
            // As mentioned earlier, CacheNodeQueuePtr->tail is NOT used for Layer 1 Doubly Linked List

            new_cacheNode->l2_next = lruEvictionTable.at(lru_insert_idx).head;
            lruEvictionTable.at(lru_insert_idx).head->l2_prev = new_cacheNode;
            lruEvictionTable.at(lru_insert_idx).head = new_cacheNode;
            if (lruEvictionTable.at(lru_insert_idx).tail == nullptr)
                lruEvictionTable.at(lru_insert_idx).tail = new_cacheNode;

            return new_cacheNode;
        }

        return nullptr;  // "Key" neither found in cache nor in persistent storage
    }

    bool cache_GET(struct KVMessage *ptr) {
        auto res = cache_GET_ptr(ptr);
        return res != nullptr;
    }

    /* ASSUMED: ptr->key and ptr->value are correctly filled in ptr
     * IMPORTANT: will calculate hash1 and hash2 here
     * */
    void cache_PUT(struct KVMessage *ptr) {
        ptr->calculate_key_hash();

        uint64_t hashTableIdx = (ptr->hash1) % HASH_TABLE_LEN;

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

            // match found, we just update the "Value"
            reader_lock.unlock();
            std::unique_lock writer_lock1(hashTable.at(hashTableIdx).rw_lock);
            std::unique_lock writer_lock2(lruEvictionTable.at(cacheNodeIter->lru_idx).rw_lock);

            if (not(cacheNodeIter->is_cache_node_valid())) {
                // The Key has either been deleted or been evicted from the cache
                if (is_full())
                    cache_eviction();
                cacheNodeIter = cacheNodeMemoryPool.acquire_instance();
                cacheNodeIter->message.hash1 = ptr->hash1;
                cacheNodeIter->message.hash2 = ptr->hash2;
                cacheNodeIter->message.set_key_fast(ptr->key);
            }

            cacheNodeIter->message.set_value_fast(ptr->value);

            // IMPORTANT: this is same as the one in "cache_GET"
            // Update the LRU list
            // No point of performing any work if cacheNodeIter is already the first node in the LRU Queue
            if (cacheNodeIter->l2_prev != nullptr) {
                cacheNodeIter->l2_prev->l2_next = cacheNodeIter->l2_next;
                if (cacheNodeIter->l2_next == nullptr)  // i.e. this is last node in LRU Queue
                    lruEvictionTable.at(cacheNodeIter->lru_idx).tail = cacheNodeIter->l2_prev;
                else
                    cacheNodeIter->l2_next->l2_prev = cacheNodeIter->l2_prev;

                cacheNodeIter->l2_prev = nullptr;
                cacheNodeIter->l2_next = lruEvictionTable.at(cacheNodeIter->lru_idx).head;
                lruEvictionTable.at(cacheNodeIter->lru_idx).head = cacheNodeIter;
            }

            return;
        }

        // TODO: As Key is not present in cache, just insert it in cache
        // IMPORTANT: It shall be written back to the storage on eviction or cleaning
        //            We set the Dirty Bit to appropriate value

        // Get the Key-Value pair in Cache
        reader_lock.unlock();

        // IMPORTANT ACTION
        if (is_full())
            cache_eviction();

        // mostly there is no possibility of creating any problem
        CacheNode *new_cacheNode = cacheNodeMemoryPool.acquire_instance();

        uint64_t lru_insert_idx = get_next_lru_queue_idx();
        new_cacheNode->set_all(ptr, nullptr, nullptr, nullptr, nullptr, lru_insert_idx, 0);

        std::unique_lock write_lock1(hashTable.at(hashTableIdx).rw_lock);
        std::unique_lock write_lock2(lruEvictionTable.at(lru_insert_idx).rw_lock);

        new_cacheNode->l1_right = hashTable.at(hashTableIdx).head;
        hashTable.at(hashTableIdx).head = new_cacheNode;
        // As mentioned earlier, CacheNodeQueuePtr->tail is NOT used for Layer 1 Doubly Linked List

        new_cacheNode->l2_next = lruEvictionTable.at(lru_insert_idx).head;
        lruEvictionTable.at(lru_insert_idx).head->l2_prev = new_cacheNode;
        lruEvictionTable.at(lru_insert_idx).head = new_cacheNode;
        if (lruEvictionTable.at(lru_insert_idx).tail == nullptr)
            lruEvictionTable.at(lru_insert_idx).tail = new_cacheNode;
    }

    /* ASSUMED: ptr->key is correctly filled in ptr
     * IMPORTANT: will calculate hash1 and hash2 here
     * */
    bool cache_DELETE(struct KVMessage *ptr) {
        ptr->calculate_key_hash();

        uint64_t hashTableIdx = (ptr->hash1) % HASH_TABLE_LEN;

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

            if (cacheNodeIter->is_cache_node_deleted()) {
                // The Key has been deleted
                return false;
            } else if (cacheNodeIter->is_cache_node_evicted()) {
                // Bring it back in cache
                CacheNode *new_cacheNodeIter = cache_GET_ptr(ptr);
                if (new_cacheNodeIter == nullptr)
                    return false;  // entry has been deleted from the storage as well

                reader_lock.unlock();
                std::unique_lock writer_lock1(hashTable.at(hashTableIdx).rw_lock);
                new_cacheNodeIter->dirty_bit = CacheNode::EnumDirtyBit::DirtyBit_TODELETE;
            } else {
                reader_lock.unlock();
                std::unique_lock writer_lock1(hashTable.at(hashTableIdx).rw_lock);
                std::unique_lock writer_lock2(lruEvictionTable.at(cacheNodeIter->lru_idx).rw_lock);
                cacheNodeIter->dirty_bit = CacheNode::EnumDirtyBit::DirtyBit_TODELETE;
                move_to_head(&lruEvictionTable.at(cacheNodeIter->lru_idx), cacheNodeIter);
            }
            return true;
        }

        // Bring it back in cache
        CacheNode *new_cacheNodeIter = cache_GET_ptr(ptr);
        if (new_cacheNodeIter == nullptr)
            return false;  // entry has been deleted from the storage as well

        reader_lock.unlock();
        std::unique_lock writer_lock1(hashTable.at(hashTableIdx).rw_lock);
        new_cacheNodeIter->dirty_bit = CacheNode::EnumDirtyBit::DirtyBit_TODELETE;
        return true;
    }

    void cache_eviction() {
        // TODO
        // Write to Persistent storage if dirty bit of a CacheNode is true
        for (int i = 0; i < 10; ++i) {
            uint64_t eqIdx = get_next_eviction_queue_idx();
            if (lruEvictionTable.at(eqIdx).head == nullptr) continue;

            uint64_t hqIdx = lruEvictionTable.at(eqIdx).tail->message.hash1 % HASH_TABLE_LEN;
            std::unique_lock writer_lock1(hashTable.at(hqIdx).rw_lock);
            std::unique_lock writer_lock2(lruEvictionTable.at(eqIdx).rw_lock);

            CacheNode *ptrToRemove = lruEvictionTable.at(eqIdx).tail;
            if (ptrToRemove->is_cache_node_evicted()) {
                writer_lock1.unlock();
                writer_lock2.unlock();
                continue;
            }

            remove_from_dll(&hashTable.at(hqIdx), ptrToRemove);
            remove_from_dll(&lruEvictionTable.at(eqIdx), ptrToRemove);

            if (ptrToRemove->is_cache_node_deleted()) {
                kvPersistentStore.delete_from_db(&ptrToRemove->message);
                cacheNodeMemoryPool.release_instance(ptrToRemove);
            } else if (ptrToRemove->is_cache_node_dirty()) {
                kvPersistentStore.write_to_db(&ptrToRemove->message);
                cacheNodeMemoryPool.release_instance(ptrToRemove);
            } else {
                // the updated value is already present in the Persistent Storage
                cacheNodeMemoryPool.release_instance(ptrToRemove);
            }

            writer_lock1.unlock();
            writer_lock2.unlock();
        }
    }

    /* Write all cached data to Persistent Storage */
    void cache_clean() {
        // TODO: Mostly will just have to call "cache_eviction" for all cache entries
        while (not is_empty()) {
            cache_eviction();
        }
    }

private:
    [[nodiscard]] inline bool is_not_full() const {
        return (
                       cacheNodeMemoryPool.memoryBlockPointers.size() * cacheNodeMemoryPool.blockSize -
                       cacheNodeMemoryPool.pool.size()
               ) < nMax;
        // return n < nMax;
    }

    [[nodiscard]] inline bool is_full() const { return not(is_not_full()); }

    [[nodiscard]] inline bool is_empty() const {
        return cacheNodeMemoryPool.memoryBlockPointers.size() * cacheNodeMemoryPool.blockSize == cacheNodeMemoryPool.pool.size();
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

        // Move ahead if only a single element is present
        while (lruEvictionTable.at(id).head == lruEvictionTable.at(id).tail) {
            index = lruEvictionIdx++;
            id = index % lruEvictionTable.size();
        }

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

        // Move ahead if only a single element is present
        while (lruEvictionTable.at(id).head == lruEvictionTable.at(id).tail) {
            index = lruTableInsertIdx++;
            id = index % lruEvictionTable.size();
        }

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

    static void move_to_head(CacheNodeQueuePtr *ptrQueue, CacheNode *ptr) {
        if (ptrQueue->head == ptr) {
            return;  // ptr is already the ptrQueue->head
        }
        remove_from_dll(ptrQueue, ptr);
        insert_to_head(ptrQueue, ptr);
    }

    /* ASSUMED: ptr will not be the first node in the Doubly Linked List (NOT Circular) */
    static void remove_from_dll(CacheNodeQueuePtr *ptrQueue, CacheNode *ptr) {
        // remove the node from Doubly Linked List - NOT Circular
        ptr->l2_prev->l2_next = ptr->l2_next;

        if (ptr->l2_next == nullptr) {
            ptrQueue->tail = ptr->l2_prev;
        } else {
            // if ptr is not the last node
            ptr->l2_next->l2_prev = ptr->l2_prev;
        }
    }

    static void insert_to_head(CacheNodeQueuePtr *ptrQueue, CacheNode *ptr) {
        ptr->l2_next = ptrQueue->head;
        ptr->l2_prev = nullptr;

        if (ptrQueue->head == nullptr) {
            // list is empty
            ptrQueue->tail = ptr;
        } else {
            // list is NON empty
            ptrQueue->head->l2_prev = ptr;
        }
        ptrQueue->head = ptr;
    }

    static bool entry_equals(KVMessage *a, KVMessage *b) {
        return (a->hash1 == b->hash1)
               && (a->hash2 == b->hash2)
               && (std::equal(a->key, a->key + 256, b->key));
    }

};

#endif // PA_4_KEY_VALUE_STORE_KVCACHE_HPP
