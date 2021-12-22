#ifndef BRMH_UTIL_HPP
#define BRMH_UTIL_HPP

#include <cstddef>

namespace brmh {

template<class T>
struct opt_ptr {
    static opt_ptr<T> none() { return opt_ptr(); }

    static opt_ptr some(T* ptr) { return opt_ptr(ptr); }

    T* unwrap_or(T* alt) const { return ptr_ ? ptr_ : alt; }

private:
    opt_ptr() : ptr_(nullptr) {}
    opt_ptr(T* ptr) : ptr_(ptr) {}

    T* ptr_;
};

} // namespace brmh

#endif // BRMH_UTIL_HPP
