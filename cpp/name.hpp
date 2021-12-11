#ifndef BRMH_NAME_HPP
#define BRMH_NAME_HPP

#include <string>
#include <unordered_set>

namespace brmh {

struct Name;

struct Names {
    Name create(const char* chars);

private:
    struct RawName {
        struct Hash {
            std::size_t operator()(const RawName& raw) const noexcept;
        };

        bool operator==(const RawName& other) const;

        const char* chars_;
    };

    std::unordered_set<RawName, RawName::Hash> names_; // FIXME: thread safety
};

struct Name {
    Name() = delete;

    bool operator==(const Name& other) const noexcept;
    std::size_t hash() const noexcept;

private:
    friend struct Names;

    explicit Name(const char* chars);

    const char* chars_;
};

} // namespace brmh

template<>
struct std::hash<brmh::Name> {
    std::size_t operator()(brmh::Name const& s) const noexcept { return s.hash(); }
};

#endif // BRMH_NAME_HPP
