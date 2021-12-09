#include "lexer.hpp"

namespace brmh {

Lexer::Lexer(const char* chars) : chars(chars) {}

optional<Lexer::Token> Lexer::peek() {
    switch (*chars) {
    case '\0': return optional<Lexer::Token>();

    case '(': return optional(Lexer::Token {Lexer::Token::Type::LPAREN, chars});
    case ')': return optional(Lexer::Token {Lexer::Token::Type::RPAREN, chars});
    case '[': return optional(Lexer::Token {Lexer::Token::Type::LBRACKET, chars});
    case ']': return optional(Lexer::Token {Lexer::Token::Type::RBRACKET, chars});
    case '{': return optional(Lexer::Token {Lexer::Token::Type::LBRACE, chars});
    case '}': return optional(Lexer::Token {Lexer::Token::Type::RBRACE, chars});

    default: return optional<Lexer::Token>();
    }
}

optional<Lexer::Token> Lexer::next() {
    const auto token = peek();
    ++chars;
    return token;
}

} // namespace brmh
