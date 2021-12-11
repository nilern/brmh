#include "name.hpp"

#include <cstring>

namespace brmh {

Name Names::create(const char* chars) {
    auto it = names_.find(Names::RawName{chars});
    if (it == names_.end()) {
        it = names_.emplace(strdup(chars)).first;
    }

    return Name(it->chars_);
}

bool Names::RawName::operator==(const RawName &other) const {
    return strcmp(chars_, other.chars_) == 0;
}

std::size_t Names::RawName::Hash::operator()(const RawName &raw) const noexcept {
    return std::hash<std::string_view>()(std::string_view(raw.chars_, strlen(raw.chars_))); // OPTIMIZE
}

Name::Name(const char* chars) : chars_(chars) {}

bool Name::operator==(const Name& other) const noexcept { return chars_ == other.chars_; }

std::size_t Name::hash() const noexcept { return std::hash<const char*>()(chars_); }

} // namespace brmh
