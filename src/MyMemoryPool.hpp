#ifndef PA_4_KEY_VALUE_STORE_MYMEMORYPOOL_HPP
#define PA_4_KEY_VALUE_STORE_MYMEMORYPOOL_HPP

#include <mutex>
#include <vector>
#include "MyDebugger.hpp"

/*
 * REFER: https://stackoverflow.com/questions/855763/is-malloc-thread-safe
 *
 * ASSUMED: type T has a default constructor which takes NO arguments
 *
 * */
template<typename T>
struct MemoryPool {
    std::mutex m;
    size_t blockSize;
    std::vector<T *> memoryBlockPointers;  // stores pointers to large BLOCKS
    std::vector<T *> pool;  // stores individual pointers to each object in the pool
    size_t n, nMax;  // values for faster "pool" "push_back" and "pop_back" operations
    bool callConstructor;

    explicit MemoryPool(bool call_constructor) :
            m(),
            blockSize{1024},
            n{0},
            nMax{2048},
            callConstructor{call_constructor} {
        memoryBlockPointers.reserve(8);
        pool.resize(2048);
    }

    void init(size_t block_size, size_t blocks_required = 8) {
        blockSize = block_size;
        memoryBlockPointers.reserve(blocks_required);
        pool.resize(block_size * blocks_required);
        allocate_one_block();
    }

    ~MemoryPool() {
        for (T *i: memoryBlockPointers) {
            if (callConstructor) {
                for (size_t j = 0; j < blockSize; ++j) {
                    (i + j)->~T();
                }
            }
            delete[] i;
        }
    }

    // size_t vec_at_rev(size_t idx) {
    //     return pool.size() - idx - 1;
    // }

    inline T *vec_pop_back() {
        --n;
        return pool.at(n);
    }

    inline void vec_push_back(T *ptr) {
        pool.at(n) = ptr;
        ++n;
    }

    inline bool vec_empty() {
        return n == 0;
    }

    T *acquire_instance() {
        m.lock();
        if (vec_empty()) {
            allocate_one_block();
        }
        T *instance_ptr = vec_pop_back();
        m.unlock();
        return instance_ptr;
    }

    /* This function ensures that no extra memory is allocated outside limit */
    T *acquire_instance_strict_limit() {
        m.lock();
        if (vec_empty()) {
            return nullptr;
        }
        T *instance_ptr = vec_pop_back();
        m.unlock();
        return instance_ptr;
    }

    void release_instance(T *ptr) {
        m.lock();
        vec_push_back(ptr);
        m.unlock();
    }

private:
    void allocate_one_block() {
        if (callConstructor)
            memoryBlockPointers.push_back(new T[blockSize]());
        else
            memoryBlockPointers.push_back(new T[blockSize]);

        nMax = memoryBlockPointers.size() * blockSize;
        if (pool.capacity() >= nMax) pool.resize(pool.capacity());
        else pool.resize(2 * nMax);

        T *new_block_ptr = memoryBlockPointers.at(memoryBlockPointers.size() - 1);

        if (new_block_ptr == nullptr) {
            // malloc failed
            log_error("NEW operator failed in \"MemoryPool.allocate_one_block()\" method");
            log_error("Exiting (status=61)");
            exit(61);
        } else {
            for (size_t i = 0; i < blockSize; ++i) {
                vec_push_back(new_block_ptr + i);
            }
        }
    }
};

#endif // PA_4_KEY_VALUE_STORE_MYMEMORYPOOL_HPP
