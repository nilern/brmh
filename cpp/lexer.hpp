#ifndef BRMH_LEXER_HPP
#define BRMH_LEXER_HPP

#include <cstdint>
#include <optional>
#include <ostream>

#include "src.hpp"
#include "pos.hpp"
#include "error.hpp"

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
            ID, FUN, I64_T
        };

        void print(std::ostream& out) const;

        Type typ;
        const char* chars;
        uintptr_t size;
        Pos pos;
    };

    class Error : public BrmhError {
    public:
        explicit Error(Pos pos);

        virtual const char* what() const noexcept override;

        Pos pos;
    };

    explicit Lexer(const Src& src);
    Lexer() = delete;

    Pos pos() const;
    optional<Token> peek();
    Token peek_some();
    optional<Token> next();
    optional<optional<Token>> match(Token::Type type);

private:
    optional<Token> lex_id();
    optional<Token> lex_int();

    char const* chars_;
    Pos pos_;
};

} // namespace brmh

#endif // BRMH_LEXER_HPP
