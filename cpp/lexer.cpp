#include "lexer.hpp"

namespace brmh {

Lexer::Lexer(const Filename filename, const char* chars) : filename_(filename), chars_(chars) {}

optional<Lexer::Token> Lexer::peek() {
    switch (*chars_) {
    case '\0': return optional<Lexer::Token>();

    case ',': return optional(Lexer::Token {Lexer::Token::Type::COMMA, chars_, Pos(filename_, index_)});
    case ';': return optional(Lexer::Token {Lexer::Token::Type::SEMICOLON, chars_, Pos(filename_, index_)});

    case '.': return optional(Lexer::Token {Lexer::Token::Type::DOT, chars_, Pos(filename_, index_)});

    case '=': return optional(Lexer::Token {Lexer::Token::Type::EQUALS, chars_, Pos(filename_, index_)});
    case ':': return optional(Lexer::Token {Lexer::Token::Type::COLON, chars_, Pos(filename_, index_)});

    case '(': return optional(Lexer::Token {Lexer::Token::Type::LPAREN, chars_, Pos(filename_, index_)});
    case ')': return optional(Lexer::Token {Lexer::Token::Type::RPAREN, chars_, Pos(filename_, index_)});
    case '[': return optional(Lexer::Token {Lexer::Token::Type::LBRACKET, chars_, Pos(filename_, index_)});
    case ']': return optional(Lexer::Token {Lexer::Token::Type::RBRACKET, chars_, Pos(filename_, index_)});
    case '{': return optional(Lexer::Token {Lexer::Token::Type::LBRACE, chars_, Pos(filename_, index_)});
    case '}': return optional(Lexer::Token {Lexer::Token::Type::RBRACE, chars_, Pos(filename_, index_)});

    default: return optional<Lexer::Token>();
    }
}

optional<Lexer::Token> Lexer::next() {
    const auto token = peek();
    ++chars_;
    ++index_;
    return token;
}

} // namespace brmh
