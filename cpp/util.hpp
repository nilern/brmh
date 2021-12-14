#ifndef BRMH_UTIL_HPP
#define BRMH_UTIL_HPP

#include <cstddef>

namespace brmh {

struct CStringEq {
    bool operator()(const char* chars1, const char* chars2) const noexcept;
};

struct CStringHash {
    std::size_t operator()(const char* raw) const noexcept;
};

} // namespace brmh

#endif // BRMH_UTIL_HPP
