#include "name.hpp"

#include <cstring>

namespace brmh {

// # Names

Names::Names() : counter_(0), by_chars_() {}

Names::~Names() { /* TODO: Free all names (in constant time!) */ }

const Name* Names::sourced(const char* chars, std::size_t size) {
    auto it = by_chars_.find(chars);
    if (it != by_chars_.end()) {
        return it->second;
    } else {
        const Name* name = fresh(chars, size);
        by_chars_.insert({name->chars_, name});
        return name;
    }
}

const Name* Names::fresh(const char* chars, std::size_t size) {
    return new Name(counter_++, false, strndup(chars, size));
}

const Name* Names::fresh() {
    return new Name(counter_++, true, "");
}

const Name* Names::freshen(const Name* name) {
    return new Name(counter_++, true, name->chars_);
}

// # Name

Name::Name(std::size_t index, bool freshened, const char* chars)
    : index_(index), freshened_(freshened), chars_(chars) {}

Name::~Name() {
    if (!freshened_) {
        delete chars_;
    }
}

void Name::print(std::ostream& out) const { out << chars_ << '$' << index_; }

bool Name::EqChars::operator()(const char* chars1, const char* chars2) const noexcept {
    return strcmp(chars1, chars2) == 0;
}

std::size_t Name::CharsHash::operator()(const char* chars) const noexcept {
    return std::hash<std::string_view>()(std::string_view(chars, strlen(chars))); // OPTIMIZE: two string traversals
}

} // namespace brmh
