#ifndef PA_4_KEY_VALUE_STORE_MYMEMORYPOOL_HPP
#define PA_4_KEY_VALUE_STORE_MYMEMORYPOOL_HPP

#include <mutex>
#include "MyVector.hpp"
#include "MyDebugger.hpp"

/*
 * REFER: https://stackoverflow.com/questions/855763/is-malloc-thread-safe
 *
 *
 *
 * */
template<typename T>
struct MemoryPool {
    size_t blockSize;
    MyVector<T*> pool;
    MyVector<T*> memoryBlockPointers;
    std::mutex m;

    explicit MemoryPool(size_t block_size) : blockSize{block_size}, pool(2 * block_size), memoryBlockPointers(8), m() {
        allocate_one_block();
    }

    ~MemoryPool() {
        for(int64_t i = 0; i < this->memoryBlockPointers.n; ++i) {
            free(this->memoryBlockPointers.at(i));
        }
    }

    T* acquire_instance() {
        m.lock();
        if(memoryBlockPointers.empty()) {
            allocate_one_block();
        }
        T *instance_ptr = memoryBlockPointers.pop_back();
        m.unlock();
        return instance_ptr;
    }

    void release_instance(T* ptr) {
        m.lock();
        memoryBlockPointers.push_back(ptr);
        m.unlock();
    }

private:
    void allocate_one_block() {
        memoryBlockPointers.push_back(reinterpret_cast<T *>(malloc(this->blockSize * sizeof(T))));
        T *new_block_ptr = memoryBlockPointers.at_rev(0);

        if(new_block_ptr == nullptr) {
            // malloc failed
            log_error("malloc failed in \"MemoryPool.allocate_one_block()\" method");
            log_error("Exiting (status=61)");
            exit(61);
        } else {
            for(int64_t i = 0; i < this->blockSize; ++i) {
                pool.push_back(new_block_ptr + i);
            }
        }
    }
};

#endif // PA_4_KEY_VALUE_STORE_MYMEMORYPOOL_HPP
