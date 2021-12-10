#include "lexer.hpp"

namespace brmh {

void Lexer::Token::print(std::ostream& out) const {
    out << "<Token ";
    {
        const char* c = chars;
        for (uintptr_t i = 0; i < size; ++c, ++i) {
            out << *c;
        }
    }
    out << " @ ";
    pos.print(out);
    out << ">";
}

Lexer::Lexer(const Filename filename, const char* chars) : chars_(chars), pos_(Pos(filename, 0)) {}

optional<Lexer::Token> Lexer::peek() {
    switch (*chars_) {
    case '\0': return optional<Lexer::Token>();

    case ',': return optional(Lexer::Token {Lexer::Token::Type::COMMA, chars_, 1, pos_});
    case ';': return optional(Lexer::Token {Lexer::Token::Type::SEMICOLON, chars_, 1, pos_});

    case '.': return optional(Lexer::Token {Lexer::Token::Type::DOT, chars_, 1, pos_});

    case '=': return optional(Lexer::Token {Lexer::Token::Type::EQUALS, chars_, 1, pos_});
    case ':': return optional(Lexer::Token {Lexer::Token::Type::COLON, chars_, 1, pos_});

    case '(': return optional(Lexer::Token {Lexer::Token::Type::LPAREN, chars_, 1, pos_});
    case ')': return optional(Lexer::Token {Lexer::Token::Type::RPAREN, chars_, 1, pos_});
    case '[': return optional(Lexer::Token {Lexer::Token::Type::LBRACKET, chars_, 1, pos_});
    case ']': return optional(Lexer::Token {Lexer::Token::Type::RBRACKET, chars_, 1, pos_});
    case '{': return optional(Lexer::Token {Lexer::Token::Type::LBRACE, chars_, 1, pos_});
    case '}': return optional(Lexer::Token {Lexer::Token::Type::RBRACE, chars_, 1, pos_});

    default: return optional<Lexer::Token>();
    }
}

optional<Lexer::Token> Lexer::next() {
    const auto token = peek();
    ++chars_;
    pos_ = Pos(pos_.filename(), pos_.index() + 1);
    return token;
}

} // namespace brmh
