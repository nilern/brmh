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
    span.print(out);
    out << ">";
}

Lexer::Lexer(const Src& src) : chars_(src.source_code()), pos_(Pos(src.filename(), 1, 1)) {}

Pos Lexer::pos() const { return pos_; }

optional<Lexer::Token> Lexer::peek() {
    while (true) {
        switch (*chars_) {
        case '\0': return optional<Lexer::Token>();

        case ',': return optional(Lexer::Token {Lexer::Token::Type::COMMA, chars_, 1, {pos_, pos_.next(*chars_)}});
        case ';': return optional(Lexer::Token {Lexer::Token::Type::SEMICOLON, chars_, 1, {pos_, pos_.next(*chars_)}});

        case '.': return optional(Lexer::Token {Lexer::Token::Type::DOT, chars_, 1, {pos_, pos_.next(*chars_)}});

        case '=': return optional(Lexer::Token {Lexer::Token::Type::EQUALS, chars_, 1, {pos_, pos_.next(*chars_)}});
        case ':': return optional(Lexer::Token {Lexer::Token::Type::COLON, chars_, 1, {pos_, pos_.next(*chars_)}});

        case '(': return optional(Lexer::Token {Lexer::Token::Type::LPAREN, chars_, 1, {pos_, pos_.next(*chars_)}});
        case ')': return optional(Lexer::Token {Lexer::Token::Type::RPAREN, chars_, 1, {pos_, pos_.next(*chars_)}});
        case '[': return optional(Lexer::Token {Lexer::Token::Type::LBRACKET, chars_, 1, {pos_, pos_.next(*chars_)}});
        case ']': return optional(Lexer::Token {Lexer::Token::Type::RBRACKET, chars_, 1, {pos_, pos_.next(*chars_)}});
        case '{': return optional(Lexer::Token {Lexer::Token::Type::LBRACE, chars_, 1, {pos_, pos_.next(*chars_)}});
        case '}': return optional(Lexer::Token {Lexer::Token::Type::RBRACE, chars_, 1, {pos_, pos_.next(*chars_)}});

        default:
            if (*chars_ == '_' && chars_[1] == '_') {
                return lex_primop();
            } else if (isspace(*chars_)) {
                pos_ = pos_.next(*chars_);
                ++chars_;
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

optional<Lexer::Token> Lexer::lex_primop() {
    Pos const start = pos_;
    Pos end = start;

    uintptr_t size = 2; // "__"
    for (const char* c = chars_ + size; isalnum(*c); ++c) {
        ++size;
        end = end.next(*c);
    }

    return optional(Lexer::Token {Lexer::Token::Type::PRIMOP, chars_, size, {start, end}});
}

optional<Lexer::Token> Lexer::lex_id() {
    Pos const start = pos_;
    Pos end = start;

    uintptr_t size = 0;
    for (const char* c = chars_; isalnum(*c); ++c) {
        ++size;
        end = end.next(*c);
    }

    const Lexer::Token::Type type = strncmp(chars_, "val", size) == 0
            ? Lexer::Token::Type::VAL
            : strncmp(chars_, "fun", size) == 0
              ? Lexer::Token::Type::FUN
              : strncmp(chars_, "if", size) == 0
                ? Lexer::Token::Type::IF
                : strncmp(chars_, "else", size) == 0
                  ? Lexer::Token::Type::ELSE
                  : strncmp(chars_, "True", size) == 0
                    ? Lexer::Token::Type::TRUE
                    : strncmp(chars_, "False", size) == 0
                      ? Lexer::Token::Type::FALSE
                      : strncmp(chars_, "bool", size) == 0
                        ? Lexer::Token::Type::BOOL
                        : strncmp(chars_, "i64", size) == 0
                          ? Lexer::Token::Type::I64_T
                          : Lexer::Token::Type::ID;
    return optional(Lexer::Token {type, chars_, size, {start, end}});
}

optional<Lexer::Token> Lexer::lex_int() {
    Pos const start = pos_;
    Pos end = start;

    uintptr_t size = 0;
    for (const char* c = chars_; isdigit(*c); ++c) {
        ++size;
        end = end.next(*c);
    }

    return optional(Lexer::Token {Lexer::Token::Type::INT, chars_, size, {start, end}});
}

void Lexer::next() {
    const auto token = peek();

    if (token) {
        chars_ += token->size;
        pos_ = token->span.end;
    }
}

void Lexer::match(Token::Type type) {
    const auto opt_tok = peek();

    if (opt_tok) {
        const auto tok = opt_tok.value();
        if (tok.typ == type) {
            chars_ += tok.size;
            pos_ = tok.span.end;
        }
    }
}

} // namespace brmh
