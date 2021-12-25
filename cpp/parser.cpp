#include "parser.hpp"

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

    const auto tok = lexer_.peek_some();
    switch (tok.typ) {
    case Lexer::Token::Type::ID: {
        params.push_back(parse_param());
        break;
    }

    default: break;
    }

    for (bool done = false; !done;) {
        const auto tok = lexer_.peek_some();
        switch (tok.typ) {
        case Lexer::Token::Type::RPAREN: {
            lexer_.next(); // discard ')'
            done = true;
            break;
        }

        case Lexer::Token::Type::COMMA: {
            lexer_.next(); // discard ','
            params.push_back(parse_param());
            break;
        }

        default: throw Error(tok.pos);
        }
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
