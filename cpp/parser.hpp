#ifndef BRMH_PARSER_HPP
#define BRMH_PARSER_HPP

#include "name.hpp"
#include "ast.hpp"
#include "lexer.hpp"
#include "error.hpp"

namespace brmh {

struct Parser {
    class Error : public BrmhError {
    public:
        explicit Error(Pos pos);

        virtual const char* what() const noexcept override;

        Pos pos;
    };

    explicit Parser(Lexer&& lexer, Names& names, type::Types& types);

    // FIXME: Proper error handling:

    ast::Program program();
    ast::Expr* expr();
    type::Type* parse_type();

private:
    ast::Expr* parse_callee();
    std::vector<ast::Expr*> parse_arglist();
    Name parse_id();
    ast::Param parse_param();
    std::vector<ast::Def*> parse_defs();
    ast::FunDef* parse_fundef();

    Lexer lexer_;
    Names& names_;
    type::Types& types_;
};

} // namespace brmh

#endif // BRMH_PARSER_HPP
