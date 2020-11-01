#ifndef PA_4_KEY_VALUE_STORE_MYVECTOR_H
#define PA_4_KEY_VALUE_STORE_MYVECTOR_H

#include <cstdlib>
#include <cstdint>

// REFER: https://github.com/fenilgmehta/Misc-Programming/blob/master/src/MyVector.c

template<typename T>
struct MyVector {
    int32_t n{}, nMax{};
    T *arr;

    explicit MyVector(size_t);

    T &at(size_t);

    bool push_back();

    bool push_back(T);

    T &pop_back();

    T &pop_back(size_t);

    void clear();

    bool resize(size_t);

    ~MyVector();
};

/* Constructor */
template<typename T>
MyVector<T>::MyVector(size_t n) {
    this->n = 0;
    this->nMax = n;
    this->arr = static_cast<T *>(malloc(n * sizeof(T)));
}

template<typename T>
inline T &MyVector<T>::at(size_t idx) {
    return this->arr[idx];
}

/* RETURNS: true if push_back was successful */
template<typename T>
bool MyVector<T>::push_back() {
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
template<typename T>
bool MyVector<T>::push_back(T val) {
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
template<typename T>
inline T &MyVector<T>::pop_back() {
    if (this->n <= 1) this->n = 0;
    else this->n -= 1;
    return this->arr[this->n];
}

/* RETURNS: the last element popped */
template<typename T>
inline T &MyVector<T>::pop_back(size_t popCount) {
    if (popCount <= 0) return this->arr[0];
    if (this->n <= popCount) this->n = 0;
    else this->n -= popCount;
    return this->arr[this->n];
}


/* Removes all elements from the vector */
template<typename T>
inline void MyVector<T>::clear() {
    this->n = 0;
}

/* RETURNS: true if resize was successful */
template<typename T>
bool MyVector<T>::resize(size_t newSize) {
    T *arrNew = static_cast<T *>(realloc(this->arr, sizeof(T) * newSize));
    if (arrNew != NULL) {
        this->arr = arrNew;
        this->nMax = newSize;
    }
    return arrNew != NULL;
}

/* Destructor */
template<typename T>
MyVector<T>::~MyVector() {
    free(this->arr);
}


#endif // PA_4_KEY_VALUE_STORE_MYVECTOR_H
