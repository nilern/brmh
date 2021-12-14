#include "util.hpp"

#include <cstring>
#include <functional>
#include <string_view>

namespace brmh {

bool CStringEq::operator()(const char* chars1, const char* chars2) const noexcept {
    return strcmp(chars1, chars2) == 0;
}

std::size_t CStringHash::operator()(const char* chars) const noexcept {
    return std::hash<std::string_view>()(std::string_view(chars, strlen(chars))); // OPTIMIZE: two string traversals
}

} // namespace brmh
