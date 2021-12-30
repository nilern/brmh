#include "parser.hpp"

#include <cstring>

namespace brmh {

// # Parser::Error

Parser::Error::Error(Pos pos_) : BrmhError(), pos(pos_) {}

const char* Parser::Error::what() const noexcept { return "ParseError"; }

// # Parser

Parser::Parser(Lexer&& lexer, Names& names, type::Types& types) : lexer_(lexer), names_(names), types_(types) {}

ast::Program Parser::program() {
    return ast::Program(std::move(parse_defs()));
}

std::vector<ast::Def*> Parser::parse_defs() {
    std::vector<ast::Def*> defs;

    while (true) {
        const auto tok = lexer_.peek();
        if (tok) {
            switch (tok->typ) {
            case Lexer::Token::Type::FUN: {
                defs.push_back(parse_fundef());
                break;
            }

            default: throw Error(tok->pos);
            }
        } else {
            return defs;
        }
    }
}

ast::FunDef* Parser::parse_fundef() {
    const Pos start_pos = lexer_.pos();

    lexer_.next(); // Discard `fun`

    const auto name = parse_id();

    std::vector<ast::Param> params;
    lexer_.match(Lexer::Token::Type::LPAREN); // Discard '('

    if (lexer_.peek_some().typ == Lexer::Token::Type::RPAREN) {
        lexer_.next(); // Discard ')'
    } else {
        params.push_back(parse_param());

        while (lexer_.peek_some().typ == Lexer::Token::Type::COMMA) {
            lexer_.next(); // Discard ','
            params.push_back(parse_param());
        }

        lexer_.match(Lexer::Token::Type::RPAREN); // Discard ')'
    }

    lexer_.match(Lexer::Token::Type::COLON); // Discard ':'
    const auto codomain = parse_type();

    lexer_.match(Lexer::Token::Type::LBRACE); // Discard '{'
    const auto body = expr();
    lexer_.match(Lexer::Token::Type::RBRACE); // Discard '}'

    const Pos end_pos = lexer_.pos();

    return new ast::FunDef(Span{start_pos, end_pos}, name, std::move(params), codomain, body);
}

ast::Param Parser::parse_param() {
    const Pos start_pos = lexer_.pos();
    const auto name = parse_id();
    lexer_.match(Lexer::Token::Type::COLON); // Discard ':'
    const auto type = parse_type();
    const Pos end_pos = lexer_.pos();
    return ast::Param{Span{start_pos, end_pos}, name, type};
}

ast::Expr* Parser::expr() {
    const auto tok = lexer_.peek_some();
    switch (tok.typ) {
    case Lexer::Token::Type::IF: {
        const auto if_tok = tok;
        lexer_.next();

        ast::Expr* const cond = expr();

        lexer_.match(Lexer::Token::Type::LBRACE); // Discard '{'

        ast::Expr* const conseq = expr();

        lexer_.match(Lexer::Token::Type::RBRACE); // Discard '}'
        lexer_.match(Lexer::Token::Type::ELSE); // Discard "else"
        lexer_.match(Lexer::Token::Type::LBRACE); // Discard '{'

        ast::Expr* const alt = expr();

        lexer_.match(Lexer::Token::Type::RBRACE); // Discard '}'

        Span span{if_tok.pos, lexer_.pos()};
        return new ast::If(span, cond, conseq, alt);
    }

    case Lexer::Token::Type::PRIMOP: {
        const auto op_tok = tok;
        lexer_.next();

        ast::PrimApp::Op const op = strncmp(op_tok.chars, "__addWI64", op_tok.size) == 0
                ? ast::PrimApp::Op::ADD_W_I64
                : strncmp(op_tok.chars, "__subWI64", op_tok.size) == 0
                  ? ast::PrimApp::Op::SUB_W_I64
                  : strncmp(op_tok.chars, "__mulWI64", op_tok.size) == 0
                    ? ast::PrimApp::Op::MUL_W_I64
                    :  throw Error(op_tok.pos);

        std::vector<ast::Expr*> args;
        lexer_.match(Lexer::Token::Type::LPAREN); // Discard '('

        if (lexer_.peek_some().typ == Lexer::Token::Type::RPAREN) {
            lexer_.next(); // Discard ')'
        } else {
            args.push_back(expr());

            while (lexer_.peek_some().typ == Lexer::Token::Type::COMMA) {
                lexer_.next(); // Discard ','
                args.push_back(expr());
            }

            lexer_.match(Lexer::Token::Type::RPAREN); // Discard ')'
        }

        Span span{op_tok.pos, lexer_.pos()};
        return new ast::PrimApp(span, op, std::move(args));
    }

    case Lexer::Token::Type::ID: {
        lexer_.next();

        Span span{tok.pos, lexer_.pos()};
        return new ast::Id(span, names_.sourced(tok.chars, tok.size));
    }

    case Lexer::Token::Type::INT: {
        lexer_.next();

        Span span{tok.pos, lexer_.pos()};
        return new ast::Int(span, tok.chars, tok.size);
    }

    default: throw Error(tok.pos);
    }
}

type::Type* Parser::parse_type() {
    const auto tok = lexer_.peek_some();
    switch (tok.typ) {
    case Lexer::Token::Type::I64_T: {
        lexer_.next();
        return types_.get_i64();
    }

    default: throw Error(tok.pos);
    }
}

Name Parser::parse_id() {
    const auto tok = lexer_.peek_some();
    switch (tok.typ) {
    case Lexer::Token::Type::ID: {
        lexer_.next();
        return names_.sourced(tok.chars, tok.size);
    }

    default: throw Error(tok.pos);
    }
}

} // namespace brmh
