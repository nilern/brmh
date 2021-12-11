#include "parser.hpp"

namespace brmh {

Parser::Parser(Lexer&& lexer, Names& names) : lexer_(lexer), names_(names) {}

std::optional<ast::Node*> Parser::expr() {
    const std::optional<Lexer::Token> tok = lexer_.peek();
    if (tok) {
        switch (tok->typ) {
        case Lexer::Token::Type::ID: {
            lexer_.next();

            Span span{tok->pos, lexer_.pos()};
            return std::optional<ast::Node*>(new ast::Id(span, names_.sourced(tok->chars, tok->size)));
        }

        default: return std::optional<ast::Node*>();
        }
    } else {
        return std::optional<ast::Node*>();
    }
}

} // namespace brmh
