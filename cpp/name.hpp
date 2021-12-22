#ifndef BRMH_NAME_HPP
#define BRMH_NAME_HPP

#include <string>
#include <unordered_map>

#include "util.hpp"

namespace brmh {

struct Names;

struct Name {
    struct Hash {
        std::size_t operator()(Name name) const noexcept;
    };

    bool operator==(Name const& other) const;

    opt_ptr<const char> src_name(Names const& names) const;
    void print(Names const& names, std::ostream& dest) const;

private:
    friend struct Names;

    explicit Name(uintptr_t id);

    uintptr_t id_;
};

struct Names {
    Names(const Names&) = delete;
    Names& operator=(const Names&) = delete;

    Name sourced(const char* chars, std::size_t size);
    Name fresh(const char* chars, std::size_t size);
    Name fresh();
    Name freshen(Name name);

    Names();
    ~Names();

    // TODO: Some sort of GC between compiler passes?

private:
    friend struct Name;

    void print_name(Name name, std::ostream& dest) const;

    // FIXME: Thread safety:

    std::size_t counter_;
    std::unordered_map<Name, const char*, Name::Hash> name_chars_;
    std::unordered_map<std::string_view, Name> by_chars_;
};

} // namespace brmh

#endif // BRMH_NAME_HPP
