#ifndef BRMH_LEXER_HPP
#define BRMH_LEXER_HPP

#include <cstdint>
#include <optional>

#include "filename.hpp"
#include "pos.hpp"

namespace brmh {

using std::optional;

struct Lexer {
    struct Token {
        enum struct Type {
            LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE,
            COMMA, SEMICOLON,
            DOT,
            EQUALS, COLON
        };

        const Type typ;
        const char* chars;
        const Pos pos;
    };

    Lexer(const Filename filename, const char* chars);
    Lexer() = delete;

    optional<Token> peek();

    optional<Token> next();

private:
    const Filename filename_;
    char const* chars_;
    uintptr_t index_;
};

} // namespace brmh

#endif // BRMH_LEXER_HPP
