#include "ast.hpp"

#include <cstring>

namespace brmh::ast {

// # Param

void Param::print(Names const& names, std::ostream& dest) const {
    name.print(names, dest);
    dest << " : ";
    type->print(names, dest);
}

// # Expr

Expr::Expr(Span sp) : span(sp) {}

// ## PrimApp

PrimApp::PrimApp(Span sp, Op op_, std::vector<Expr*>&& args_) : Expr(sp), op(op_), args(std::move(args_)) {}

void PrimApp::print(Names const& names, std::ostream& dest) const {
    print_op(op, dest);

    dest << '(';

    auto arg = args.begin();
    if (arg != args.end()) {
        (*arg)->print(names, dest);
        ++arg;

        for (; arg != args.end(); ++arg) {
            dest << ", ";
            (*arg)->print(names, dest);
        }
    }

    dest << ')';
}

void PrimApp::print_op(Op op, std::ostream &dest) {
    switch (op) {
    case Op::ADD_W_I64: dest << "__addWI64"; break;
    case Op::SUB_W_I64: dest << "__subWI64"; break;
    case Op::MUL_W_I64: dest << "__mulWI64";
    }
}

// ## Const

Const::Const(Span sp) : Expr(sp) {}

// ## Id

Id::Id(Span span, Name n) : Expr(span), name(n) {}

void Id::print(Names const& names, std::ostream& dest) const { name.print(names, dest); }

// ## Int

Int::Int(Span span, const char* chars, std::size_t size) : Const(span), digits(strndup(chars, size)) {}

void Int::print(Names const&, std::ostream& dest) const { dest << digits; }

// # Def

Def::Def(Span span_) : span(span_) {}

// ## FunDef

FunDef::FunDef(Span span, Name name_, std::vector<Param>&& params_, type::Type* codomain_, Expr* body_)
    : Def(span), name(name_), params(params_), codomain(codomain_), body(body_) {}

void FunDef::print(Names const& names, std::ostream& dest) const {
    dest << "fun ";
    name.print(names, dest);

    dest << " (";

    auto param = params.begin();
    if (param != params.end()) {
        param->print(names, dest);
        ++param;

        for (; param != params.end(); ++param) {
            dest << ", ";
            param->print(names, dest);
        }
    }

    dest << ") : ";

    codomain->print(names, dest);

    dest << " {" << std::endl;

    dest << "    ";
    body->print(names, dest);

    dest << std::endl << '}';
}

// # Program

Program::Program(std::vector<Def*>&& defs_) : defs(std::move(defs_)) {}

void Program::print(Names const& names, std::ostream& dest) const {
    for (const auto def : defs) {
        def->print(names, dest);
        dest << std::endl << std::endl;
    }
}

} // namespace brmh
