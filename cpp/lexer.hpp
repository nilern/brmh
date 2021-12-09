#ifndef BRMH_LEXER_HPP
#define BRMH_LEXER_HPP

#include <optional>

namespace brmh {

using std::optional;

struct Lexer {
    Lexer() = delete;

    Lexer(const char* chars);

    optional<char> peek();

    optional<char> next();

private:
    char const* chars;
};

} // namespace brmh

#endif // BRMH_LEXER_HPP
