#include "util.hpp"
#include "name.hpp"

#include <cstring>
#include <ostream>

namespace brmh {

// # Names

Names::Names() : counter_(0), name_chars_(), by_chars_() {}

Names::~Names() { /* TODO: Free all C strings */ }

Name Names::sourced(const char* chars, std::size_t size) {
    auto it = by_chars_.find(std::string_view(chars, size));
    if (it != by_chars_.end()) {
        return it->second;
    } else {
        const Name name = fresh();
        const char* const new_chars = strndup(chars, size);
        name_chars_.insert({name, new_chars});
        by_chars_.insert({new_chars, name});
        return name;
    }
}

Name Names::fresh(const char* chars, std::size_t size) {
    const Name name = fresh();
    name_chars_.insert({name, strndup(chars, size)});
    return name;
}

Name Names::fresh() { return Name(counter_++); }

Name Names::freshen(Name name) {
    const Name new_name = fresh();

    auto it = name_chars_.find(name);
    if (it != name_chars_.end()) {
        name_chars_.insert({new_name, it->second});
    }

    return new_name;
}

void Names::print_name(Name name, std::ostream& dest) const {
    auto it = name_chars_.find(name);
    if (it != name_chars_.end()) {
        dest << it->second;
    }

    dest << '$' << name.id_;
}

// # Name

std::size_t Name::Hash::operator()(Name name) const noexcept { return std::hash<std::uintptr_t>()(name.id_); }

Name::Name(uintptr_t id) : id_(id) {}

bool Name::operator==(const Name& other) const { return id_ == other.id_; }

opt_ptr<const char> Name::src_name(const Names &names) const {
    auto it = names.name_chars_.find(*this);
    return it != names.name_chars_.end() ? opt_ptr<const char>::some(it->second) : opt_ptr<const char>::none();
}

void Name::print(Names const& names, std::ostream& dest) const { names.print_name(*this, dest); }

} // namespace brmh
