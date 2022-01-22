#ifndef BRMH_UTIL_HPP
#define BRMH_UTIL_HPP

#include <cstddef>

namespace brmh {

template<class T>
struct opt_ptr {
    static opt_ptr none() { return opt_ptr(); }

    static opt_ptr some(T* ptr) { return opt_ptr(ptr); }

    bool is_none() const { return !ptr_; }

    template<typename R, typename F, typename G>
    R match(F f, G g) const { return ptr_ ? f(ptr_) : g(); }

    T* unwrap() const {
        assert(ptr_);
        return ptr_;
    }

    T* unwrap_or(T* alt) const { return ptr_ ? ptr_ : alt; }

    template<typename R, typename F>
    opt_ptr<R> map(F f) const { return ptr_ ? opt_ptr<R>::some(f(ptr_)) : opt_ptr<R>::none(); }

    template<typename F>
    void iter(F f) const {
        if (ptr_) {
            f(ptr_);
        }
    }

private:
    opt_ptr() : ptr_(nullptr) {}
    opt_ptr(T* ptr) : ptr_(ptr) {}

    T* ptr_;
};

} // namespace brmh

#endif // BRMH_UTIL_HPP
