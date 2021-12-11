#ifndef BRMH_LEXER_HPP
#define BRMH_LEXER_HPP

#include <cstdint>
#include <optional>
#include <ostream>

#include "src.hpp"
#include "pos.hpp"

namespace brmh {

using std::optional;

struct Lexer {
    struct Token {
        enum struct Type {
            LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE,
            COMMA, SEMICOLON,
            DOT,
            EQUALS, COLON,
            INT,
            ID
        };

        void print(std::ostream& out) const;

        Type typ;
        const char* chars;
        uintptr_t size;
        Pos pos;
    };

    explicit Lexer(const Src& src);
    Lexer() = delete;

    optional<Token> peek();

    optional<Token> next();

private:
    optional<Token> lex_id();
    optional<Token> lex_int();

    char const* chars_;
    Pos pos_;
};

} // namespace brmh

#endif // BRMH_LEXER_HPP
