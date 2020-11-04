#ifndef PA_4_KEY_VALUE_STORE_MYVECTOR_H
#define PA_4_KEY_VALUE_STORE_MYVECTOR_H

#include <cstdlib>
#include <cstdint>

// REFER: https://github.com/fenilgmehta/Misc-Programming/blob/master/src/MyVector.c

template<typename T>
struct MyVector {
    size_t n{}, nMax{};
    T *arr;

    /* Constructor */
    explicit MyVector(const size_t n) : n{0}, nMax{n} {
        this->arr = static_cast<T *>(malloc(n * sizeof(T)));
    }

    inline T &at(const size_t idx) {
        return this->arr[idx];
    }

    /* RETURNS: true if push_back was successful */
    bool push_back() {
        if (this->n == this->nMax) {
            if (not(this->resize(this->nMax * 2))) {
                /* if resize fails */
                return false;
            }
        }
        this->n += 1;
        return true;
    }

    /* RETURNS: true if push_back was successful */
    bool push_back(T val) {
        if (this->n == this->nMax) {
            if (not(this->resize(this->nMax * 2))) {
                /* if resize fails */
                return false;
            }
        }
        this->arr[this->n] = val;
        this->n += 1;
        return true;
    }

    /* RETURNS: the last element popped */
    inline T &pop_back() {
        if (this->n <= 1) this->n = 0;
        else this->n -= 1;
        return this->arr[this->n];
    }

    /* RETURNS: the last element popped */
    inline T &pop_back(const size_t popCount) {
        if (popCount <= 0) return this->arr[0];
        if (this->n <= popCount) this->n = 0;
        else this->n -= popCount;
        return this->arr[this->n];
    }

    /* Removes all elements from the vector */
    inline void clear() {
        this->n = 0;
    }

    /* RETURNS: true if resize was successful */
    bool resize(const size_t newSize) {
        T *arrNew = static_cast<T *>(realloc(this->arr, sizeof(T) * newSize));
        if (arrNew != NULL) {
            this->arr = arrNew;
            this->nMax = newSize;
        }
        return arrNew != NULL;
    }

    /* Destructor */
    ~MyVector() {
        free(this->arr);
    }
};


#endif // PA_4_KEY_VALUE_STORE_MYVECTOR_H
