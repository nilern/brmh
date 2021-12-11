#ifndef BRMH_NAME_HPP
#define BRMH_NAME_HPP

#include <string>
#include <unordered_map>

namespace brmh {

struct Names;

struct Name {
    Name() = delete;
    Name(const Name&) = delete;
    Name(const Name&&) = delete;
    Name& operator=(const Name&) = delete;
    Name& operator=(const Name&&) = delete;

    ~Name();

    void print(std::ostream& out) const;

private:
    friend struct Names;

    struct EqChars {
        bool operator()(const char* chars1, const char* chars2) const noexcept;
    };

    struct CharsHash {
        std::size_t operator()(const char* raw) const noexcept;
    };

    Name(std::size_t index, bool freshened, const char* chars);

    std::size_t index_;
    bool freshened_;
    const char* chars_;
};

struct Names {
    Names(const Names&) = delete;
    Names& operator=(const Names&) = delete;

    const Name* sourced(const char* chars, std::size_t size);
    const Name* fresh(const char* chars, std::size_t size);
    const Name* fresh();
    const Name* freshen(const Name* name);

    Names();
    ~Names();

    // TODO: Some sort of GC between compiler passes?

private:
    // FIXME: Thread safety:
    std::size_t counter_;
    std::unordered_map<const char*, const Name*, Name::CharsHash, Name::EqChars> by_chars_;
};

} // namespace brmh

#endif // BRMH_NAME_HPP
