#ifndef PA_4_KEY_VALUE_STORE_MYVECTOR_H
#define PA_4_KEY_VALUE_STORE_MYVECTOR_H

#include <cstdlib>
#include <cstdint>

/*
 * REFER: https://github.com/fenilgmehta/Misc-Programming/blob/master/src/MyVector.c
 *
 * MyVector is 3 times faster than std::vector when inserting 10'00'00'000
 * elements using push_back(...) function with initial capacity set to 10'000
 *
 * CAUTION: the constructor and destructor will NOT be called on actual "push_back"
 *          and "pop_back" operation.
 *        : the destructor will not be called for any object in the "MyVector" object
 *          when "~MyVector()" is invoked
 *        : datatype T should be safe to work with "malloc", "realloc" and "free"
 *
 * */
template<typename T>
struct MyVector {
    size_t n{}, nMax{};
    T *arr;

    /* Constructor */
    explicit MyVector(const size_t n_capacity) : n{0}, nMax{n_capacity} {
        this->arr = static_cast<T *>(malloc(n_capacity * sizeof(T)));
    }

    /* Destructor */
    ~MyVector() {
        free(this->arr);
    }

    // REFER: https://stackoverflow.com/questions/11313517/malloc-and-constructors-in-c?lq=1
    template <class... Args>
    inline void call_constructor(const size_t idx, Args&&... args) {
        new(arr + idx) T(args...);
    }

    template <class... Args>
    void call_constructor(size_t first_idx, const size_t last_idx, Args&&... args) {
        for(; first_idx != last_idx; ++first_idx) {
            new(arr + first_idx) T(args...);
        }
    }

    template <class... Args>
    void call_constructor_all(Args&&... args) {
        call_constructor(0, n, args...);
    }

    inline void call_destructor(const size_t idx) {
        (arr + idx)->~T();
    }

    void call_destructor(size_t first_idx, const size_t last_idx) {
        for(; first_idx != last_idx; ++first_idx) {
            (arr + first_idx)->~T();
        }
    }

    void call_destructor_all() {
        call_destructor(0, n);
    }

    inline T &at(const size_t idx) {
        return this->arr[idx];
    }

    /* Reverse indexing, i.e. 0 points to last element(i.e. index = n-1) of MyVector */
    inline T &at_rev(const size_t idx) {
        return this->arr[n-1-idx];
    }

    inline const T &at(const size_t idx) const {
        return this->arr[idx];
    }

    // TODO: check who all are calling push_back and see if object are properly constructed or not ?

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
    bool push_back(const T &val) {
        if (this->n == this->nMax) {
            if (not(this->resize(this->nMax * 4))) {
                /* if resize fails */
                return false;
            }
        }
        this->arr[this->n] = val;
        this->n += 1;
        return true;
    }

    template <class... Args>
    bool emplace_back(Args&&... args) {
        if (this->n == this->nMax) {
            if (not(this->resize(this->nMax * 4))) {
                /* if resize fails */
                return false;
            }
        }
        call_constructor(this->n, args...);
        this->n += 1;
        return true;
    }

    /* RETURNS: the last element popped */
    inline T &pop_back() {
        if (this->n <= 1) this->n = 0;
        else this->n -= 1;

        // REFER: https://www.geeksforgeeks.org/possible-call-constructor-destructor-explicitly/
        // explicitly call the destructor
        call_destructor(this->n);
        return this->arr[this->n];
    }

    /* RETURNS: the last element popped
     * CAUTION: the destructor will only be called for the last element popped
     * */
    inline T &pop_back(const size_t popCount) {
        if (popCount <= 0) return this->arr[0];
        if (this->n <= popCount) this->n = 0;
        else this->n -= popCount;

        // explicitly call the destructor for the last element popped
        call_destructor(this->n, this->n + popCount);
        return this->arr[this->n];
    }

    /* Removes all elements from the vector */
    inline void clear() {
        this->n = 0;
    }

    void fill(const T& val) {
        for(size_t i = 0; i < n; ++i) arr[i] = val;
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

    inline void expand_to_full_capacity() {
        this->n = this->nMax;
    }

    [[nodiscard]] inline size_t size() const {
        return n;
    }

    [[nodiscard]] inline bool empty() const {
        return this-n == 0;
    }

};


#endif // PA_4_KEY_VALUE_STORE_MYVECTOR_H
