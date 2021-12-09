#ifndef BRMH_LEXER_HPP
#define BRMH_LEXER_HPP

#include <optional>

namespace brmh {

using std::optional;

struct Lexer {
    struct Token {
        enum struct Type {
            LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE
        };

        const Type typ;
        const char* chars;
    };

    Lexer(const char* chars);

    optional<Token> peek();

    optional<Token> next();

private:
    char const* chars;
};

} // namespace brmh

#endif // BRMH_LEXER_HPP
