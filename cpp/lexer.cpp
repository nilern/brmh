#include "lexer.hpp"

#include <cstring>

namespace brmh {

// # Lexer::Error

Lexer::Error::Error(Pos pos_) : BrmhError(), pos(pos_) {}

const char* Lexer::Error::what() const noexcept { return "LexError"; }

// # Lexer

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

Lexer::Lexer(const Src& src) : chars_(src.source_code()), pos_(Pos(src.filename(), 0)) {}

Pos Lexer::pos() const { return pos_; }

optional<Lexer::Token> Lexer::peek() {
    while (true) {
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

        default:
            if (isspace(*chars_)) {
                ++chars_;
                pos_ = Pos(pos_.filename(), pos_.index() + 1);
                continue;
            } else if (isalpha(*chars_)) {
                return lex_id();
            } else if (isdigit(*chars_)) {
                return lex_int();
            } else {
                return optional<Lexer::Token>();
            }
        }
    }
}

Lexer::Token Lexer::peek_some() {
    const optional<Token> tok = peek();
    if (tok) {
        return tok.value();
    } else {
        throw Error(pos());
    }
}

optional<Lexer::Token> Lexer::lex_id() {
    uintptr_t size = 0;

    for (const char* c = chars_; isalpha(*c); ++c) {
        ++size;
    }

    const Lexer::Token::Type type = strncmp(chars_, "fun", size) == 0
            ? Lexer::Token::Type::FUN
            : strncmp(chars_, "int", size) == 0
              ? Lexer::Token::Type::INT_T
              : Lexer::Token::Type::ID;
    return optional(Lexer::Token {type, chars_, size, pos_});
}

optional<Lexer::Token> Lexer::lex_int() {
    uintptr_t size = 0;

    for (const char* c = chars_; isdigit(*c); ++c) {
        ++size;
    }

    return optional(Lexer::Token {Lexer::Token::Type::INT, chars_, size, pos_});
}

optional<Lexer::Token> Lexer::next() {
    const auto token = peek();

    if (token) {
        chars_ += token->size;
        pos_ = Pos(pos_.filename(), pos_.index() + token->size);
    }

    return token;
}

optional<optional<Lexer::Token>> Lexer::match(Token::Type type) {
    const auto opt_tok = peek();
    if (!opt_tok) {
        return optional<optional<Lexer::Token>>();
    } else {
        const auto tok = opt_tok.value();
        if (tok.typ == type) {
            chars_ += tok.size;
            pos_ = Pos(pos_.filename(), pos_.index() + tok.size);
            return optional(optional(tok));
        } else {
            return optional(optional<Lexer::Token>());
        }

    }
}

} // namespace brmh
