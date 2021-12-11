#ifndef BRMH_PARSER_HPP
#define BRMH_PARSER_HPP

#include "name.hpp"
#include "ast.hpp"
#include "lexer.hpp"

namespace brmh {

struct Parser {
    explicit Parser(Lexer&& lexer, Names& names);

    std::optional<ast::Node*> expr();

private:
    Lexer lexer_;
    Names& names_;
};

} // namespace brmh

#endif // BRMH_PARSER_HPP
