#ifndef PA_4_KEY_VALUE_STORE_MYMEMORYPOOL_HPP
#define PA_4_KEY_VALUE_STORE_MYMEMORYPOOL_HPP

#include <mutex>
#include "MyVector.hpp"
#include "MyDebugger.hpp"

/*
 * REFER: https://stackoverflow.com/questions/855763/is-malloc-thread-safe
 *
 * ASSUMED: type T has a default constructor which takes not arguments
 *
 * */
template<typename T>
struct MemoryPool {
    size_t blockSize;
    MyVector<T *> pool;
    MyVector<T *> memoryBlockPointers;
    std::mutex m;

    MemoryPool() : blockSize{1024}, pool(2048), memoryBlockPointers(4), m() {}

    void init(size_t block_size) {
        blockSize = block_size;
        pool.resize(2 * block_size);
        memoryBlockPointers.resize(4);
        allocate_one_block();
    }

    ~MemoryPool() {
        for (int64_t i = 0; i < this->memoryBlockPointers.n; ++i) {
            for (int64_t j = 0; j < blockSize; ++j) {
                // Call Destructor
                (memoryBlockPointers.at(i) + j)->~T();
            }
            free(this->memoryBlockPointers.at(i));
        }
    }

    T *acquire_instance() {
        m.lock();
        if (memoryBlockPointers.empty()) {
            allocate_one_block();
        }
        T *instance_ptr = memoryBlockPointers.pop_back();
        m.unlock();
        return instance_ptr;
    }

    /* This function ensures that no extra memory is allocated outside limit */
    T *acquire_instance_strict_limit() {
        m.lock();
        if (memoryBlockPointers.empty()) {
            return nullptr;
        }
        T *instance_ptr = memoryBlockPointers.pop_back();
        m.unlock();
        return instance_ptr;
    }

    void release_instance(T *ptr) {
        m.lock();
        memoryBlockPointers.push_back(ptr);
        m.unlock();
    }

    template<class... Args>
    void call_constructor(T *ptr, Args &&... args) {
        new(ptr) T(args...);
    }

    void call_destructor(T *ptr) {
        ptr->~T();
    }

private:
    void allocate_one_block() {
        memoryBlockPointers.push_back(reinterpret_cast<T *>(malloc(this->blockSize * sizeof(T))));
        T *new_block_ptr = memoryBlockPointers.at_rev(0);

        if (new_block_ptr == nullptr) {
            // malloc failed
            log_error("MALLOC failed in \"MemoryPool.allocate_one_block()\" method");
            log_error("Exiting (status=61)");
            exit(61);
        } else {
            for (int64_t i = 0; i < this->blockSize; ++i) {
                // Call Default Constructor
                pool.push_back(new_block_ptr + i);
                new(new_block_ptr + i) T();
            }
        }
    }
};

#endif // PA_4_KEY_VALUE_STORE_MYMEMORYPOOL_HPP
