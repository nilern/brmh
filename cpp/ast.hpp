#ifndef BRMH_AST_HPP
#define BRMH_AST_HPP

#include <vector>

#include "span.hpp"
#include "name.hpp"
#include "type.hpp"
#include "fast.hpp"
#include "typeenv.hpp"

namespace brmh::ast {

struct Stmt;

// # Param

struct Param {
    Span span;
    Name name;
    type::Type* type;

    Name declare(TypeEnv& env) const;

    void print(Names const& names, std::ostream& dest) const;
};

// # Exprs

struct Expr {
    explicit Expr(Span span);

    virtual fast::Expr* type_of(fast::Program& program, TypeEnv& env) const = 0;
    fast::Expr* check(fast::Program& program, TypeEnv& env, type::Type* type) const;

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    Span span;
};

struct Block : public Expr {
    std::vector<Stmt*> stmts;
    Expr* body;

    Block(Span span, std::vector<Stmt*>&& stmts_, Expr* body_)
        : Expr(span), stmts(stmts_), body(body_) {}

    virtual fast::Expr* type_of(fast::Program& program, TypeEnv& env) const override;

    virtual void print(Names const& names, std::ostream& dest) const override;
};

struct If : public Expr {
    Expr* cond;
    Block* conseq;
    Block* alt;

    If(Span span, Expr* cond, Block* conseq, Block* alt);

    virtual fast::Expr* type_of(fast::Program& program, TypeEnv& env) const override;

    virtual void print(Names const& names, std::ostream& dest) const override;
};

struct Call : public Expr {
    Expr* callee;
    std::vector<Expr*> args;

    Call(Span span, Expr* callee_, std::vector<Expr*>&& args_)
        : Expr(span), callee(callee_), args(std::move(args_)) {}

    virtual fast::Expr* type_of(fast::Program& program, TypeEnv& env) const override;

    virtual void print(Names const& names, std::ostream& dest) const override {
        callee->print(names, dest);

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
};

struct PrimApp : public Expr {
    enum struct Op {
        ADD_W_I64, SUB_W_I64, MUL_W_I64,
        EQ_I64
    };

    PrimApp(Span span, Op op, std::vector<Expr*>&& args);

    virtual fast::Expr* type_of(fast::Program& program, TypeEnv& env) const override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    Op op;
    std::vector<Expr*> args; // OPTIMIZE

private:
    static void print_op(Op op, std::ostream& dest);
};

struct Id : public Expr {
    Id(Span pos, Name name);

    virtual fast::Expr* type_of(fast::Program& program, TypeEnv& env) const override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    Name name;
};

struct Const : public Expr {
    explicit Const(Span span);
};

struct Bool : public Const {
    Bool(Span span, bool v) : Const(span), value(v) {}

    virtual fast::Expr* type_of(fast::Program& program, TypeEnv& env) const override;

    virtual void print(Names const&, std::ostream& dest) const override {
        dest << (value ? "True" : "False");
    }

    bool value;
};

struct Int : public Const {
    Int(Span pos, const char* chars, std::size_t size);

    virtual fast::Expr* type_of(fast::Program& program, TypeEnv& env) const override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    const char* digits;
};

// # Patterns

struct Pat {
    Span span;

    explicit Pat(Span span_) : span(span_) {}

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    virtual fast::Pat* type_of(fast::Program& program, TypeEnv& env) const = 0;
    fast::Pat* check(fast::Program& program, TypeEnv& env, type::Type* type) const;
};

// ## IdPat

struct IdPat : public Pat {
    Name name;

    IdPat(Span span, Name name_) : Pat(span), name(name_) {}

    virtual void print(Names const& names, std::ostream& dest) const override { name.print(names, dest); }

    virtual fast::Pat* type_of(fast::Program& program, TypeEnv& env) const override;
};

// # Statements

struct Stmt {
    Span span;

    explicit Stmt(Span span_) : span(span_) {}

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    virtual std::pair<fast::Stmt*, TypeEnv> check(fast::Program& program, TypeEnv& env) const = 0;
};

// ## Val

struct Val : public Stmt {
    Pat* pat;
    Expr* val_expr;

    Val(Span span, Pat* pat_, Expr* val_expr_)
        : Stmt(span), pat(pat_), val_expr(val_expr_) {}

    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual std::pair<fast::Stmt*, TypeEnv> check(fast::Program& program, TypeEnv& env) const override;
};

// # Defs

struct Def {
    explicit Def(Span span);

    virtual void declare(TypeEnv& env) = 0;
    virtual fast::Def* check(fast::Program& program, TypeEnv& env) = 0;

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    Span span;
};

struct FunDef : public Def {
    Name name;
    std::vector<Param> params;
    type::Type* codomain;
    Block* body;

    FunDef(Span span, Name name, std::vector<Param>&& params, type::Type* codomain, Block* body);

    std::vector<type::Type*> domain() const;

    virtual void declare(TypeEnv& env) override;
    virtual fast::Def* check(fast::Program& program, TypeEnv& env) override;

    virtual void print(Names const& names, std::ostream& dest) const override;
};

// # Program

struct Program {
    explicit Program(std::vector<Def*>&& defs);

    fast::Program check(Names& names, type::Types& types);

    void print(Names const& names, std::ostream& dest) const;

    std::vector<Def*> defs;
};

} // namespace brmh::ast

#endif // BRMH_AST_HPP
