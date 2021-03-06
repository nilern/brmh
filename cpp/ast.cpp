#include "ast.hpp"

#include <cstring>

namespace brmh::ast {

// # Expr

Expr::Expr(Span sp) : span(sp) {}

// ## Block

void Block::print(Names const& names, std::ostream& dest) const {
    dest << "{\n";

    for (Stmt* stmt : stmts) {
        dest << "        ";
        stmt->print(names, dest);
        dest << ";\n";
    }

    dest << "        ";
    body->print(names, dest);

    dest << "\n    }";
}

// # If

If::If(Span span_, Expr* cond_, Block* conseq_, Block* alt_) : Expr(span_), cond(cond_), conseq(conseq_), alt(alt_) {}

void If::print(Names const& names, std::ostream& dest) const {
    dest << "if ";
    cond->print(names, dest);
    dest << " {\n        ";
    conseq->print(names, dest);
    dest << "\n    } else {\n        ";
    alt->print(names, dest);
    dest << "\n    }";
}

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
    case Op::MUL_W_I64: dest << "__mulWI64"; break;
    case Op::EQ_I64: dest << "__eqI64";
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

// # Statements

// ## Val

void Val::print(const Names &names, std::ostream &dest) const {
    dest << "val ";
    pat->print(names, dest);
    dest << " = ";
    val_expr->print(names, dest);
}

// # Def

Def::Def(Span span_) : span(span_) {}

// ## FunDef

void FunDef::print(Names const& names, std::ostream& dest) const {
    dest << "fun ";
    name.print(names, dest);

    dest << " (";

    auto param = params.begin();
    if (param != params.end()) {
        (*param)->print(names, dest);
        ++param;

        for (; param != params.end(); ++param) {
            dest << ", ";
            (*param)->print(names, dest);
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
