#ifndef BRMH_LEXER_HPP
#define BRMH_LEXER_HPP

#include <cstdint>
#include <optional>
#include <ostream>

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

        void print(std::ostream& out) const;

        const Type typ;
        const char* const chars;
        const uintptr_t size;
        const Pos pos;
    };

    Lexer(const Filename filename, const char* chars);
    Lexer() = delete;

    optional<Token> peek();

    optional<Token> next();

private:
    char const* chars_;
    Pos pos_;
};

} // namespace brmh

#endif // BRMH_LEXER_HPP
