#include "parser.hpp"

#include <cstring>

namespace brmh {

// # Parser::Error

Parser::Error::Error(Pos pos_) : BrmhError(), pos(pos_) {}

const char* Parser::Error::what() const noexcept { return "ParseError"; }

// # Parser

Parser::Parser(Lexer&& lexer, Names& names, type::Types& types) : lexer_(lexer), names_(names), types_(types) {}

ast::Program Parser::program() {
    return ast::Program(parse_defs());
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

    const auto body = parse_block();

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

// block ::= '{' (stmt ';')* expr '}'
ast::Block* Parser::parse_block() {
    auto const start_pos = lexer_.pos();

    // Discard '{':
    assert(lexer_.peek_some().typ == Lexer::Token::Type::LBRACE);
    lexer_.next();

    std::vector<ast::Stmt*> stmts;
    while (lexer_.peek_some().typ == Lexer::Token::Type::VAL) {
        stmts.push_back(parse_stmt());
        lexer_.match(Lexer::Token::Type::SEMICOLON); // Discard ';'
    }

    auto const body = expr();

    lexer_.match(Lexer::Token::Type::RBRACE); // Discard '}'

    Span span{start_pos, lexer_.pos()};
    return new ast::Block(span, std::move(stmts), body);
}

// expr ::= callee arglist*
ast::Expr* Parser::expr() {
    ast::Expr* expr = parse_callee();
    Pos const start_pos = expr->span.start;

    while (lexer_.peek_some().typ == Lexer::Token::Type::LPAREN) {
        std::vector<ast::Expr*> args = parse_arglist();
        const Pos end_pos = lexer_.pos();
        expr = new ast::Call(Span{start_pos, end_pos}, expr, std::move(args));
    }

    return expr;
}

std::vector<ast::Expr*> Parser::parse_arglist() {
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

    return args;
}

ast::Expr* Parser::parse_callee() {
    const auto tok = lexer_.peek_some();
    switch (tok.typ) {
    case Lexer::Token::Type::LBRACE: return parse_block();

    case Lexer::Token::Type::IF: {
        auto const if_tok = tok;

        lexer_.next(); // Discard "if"
        auto const cond = expr();
        auto const conseq = parse_block();
        lexer_.match(Lexer::Token::Type::ELSE); // Discard "else"
        auto const alt = parse_block();

        Span span{if_tok.pos, lexer_.pos()};
        return new ast::If(span, cond, conseq, alt);
    }

    case Lexer::Token::Type::PRIMOP: {
        const auto op_tok = tok;
        lexer_.next();

        // TODO: Check op existence in typing, not parsing:
        ast::PrimApp::Op const op = strncmp(op_tok.chars, "__addWI64", op_tok.size) == 0
                ? ast::PrimApp::Op::ADD_W_I64
                : strncmp(op_tok.chars, "__subWI64", op_tok.size) == 0
                  ? ast::PrimApp::Op::SUB_W_I64
                  : strncmp(op_tok.chars, "__mulWI64", op_tok.size) == 0
                    ? ast::PrimApp::Op::MUL_W_I64
                    : strncmp(op_tok.chars, "__eqI64", op_tok.size) == 0
                      ? ast::PrimApp::Op::EQ_I64
                      :  throw Error(op_tok.pos);

        std::vector<ast::Expr*> args = parse_arglist();

        Span span{op_tok.pos, lexer_.pos()};
        return new ast::PrimApp(span, op, std::move(args));
    }

    case Lexer::Token::Type::ID: {
        lexer_.next();

        Span span{tok.pos, lexer_.pos()};
        return new ast::Id(span, names_.sourced(tok.chars, tok.size));
    }

    case Lexer::Token::Type::TRUE: {
        lexer_.next();

        Span span{tok.pos, lexer_.pos()};
        return new ast::Bool(span, true);
    }

    case Lexer::Token::Type::FALSE: {
        lexer_.next();

        Span span{tok.pos, lexer_.pos()};
        return new ast::Bool(span, false);
    }

    case Lexer::Token::Type::INT: {
        lexer_.next();

        Span span{tok.pos, lexer_.pos()};
        return new ast::Int(span, tok.chars, tok.size);
    }

    default: throw Error(tok.pos);
    }
}

ast::Pat* Parser::parse_pat() {
    const auto tok = lexer_.peek_some();
    switch (tok.typ) {
    case Lexer::Token::Type::ID: {
        lexer_.next();

        Span span{tok.pos, lexer_.pos()};
        return new ast::IdPat(span, names_.sourced(tok.chars, tok.size));
    }

    default: throw Error(tok.pos);
    }
}

ast::Stmt* Parser::parse_stmt() {
    const auto tok = lexer_.peek_some();
    switch (tok.typ) {
    case Lexer::Token::Type::VAL: { // 'val' pat '=' expr
        auto const start_pos = tok.pos;
        lexer_.next(); // Discard "val"
        auto const pat = parse_pat();
        lexer_.match(Lexer::Token::Type::EQUALS); // Discard '='
        auto const val_expr = expr();

        Span span{start_pos, val_expr->span.end};
        return new ast::Val(span, pat, val_expr);
    }

    default: throw Error(tok.pos);
    }
}

type::Type* Parser::parse_type() {
    const auto tok = lexer_.peek_some();
    switch (tok.typ) {
    case Lexer::Token::Type::BOOL: {
        lexer_.next();
        return types_.get_bool();
    }

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
