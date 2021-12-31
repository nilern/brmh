#ifndef BRMH_AST_HPP
#define BRMH_AST_HPP

#include <vector>

#include "span.hpp"
#include "name.hpp"
#include "type.hpp"
#include "fast.hpp"
#include "typeenv.hpp"

namespace brmh::ast {

// # Param

struct Param {
    Span span;
    Name name;
    type::Type* type;

    void declare(TypeEnv& env) const;

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

struct If : public Expr {
    If(Span span, Expr* cond, Expr* conseq, Expr* alt);

    virtual fast::Expr* type_of(fast::Program& program, TypeEnv& env) const override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    Expr* cond;
    Expr* conseq;
    Expr* alt;
};

struct PrimApp : public Expr {
    enum struct Op {
        ADD_W_I64, SUB_W_I64, MUL_W_I64
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

// # Defs

struct Def {
    explicit Def(Span span);

    virtual void declare(TypeEnv& env) = 0;
    virtual fast::Def* check(fast::Program& program, TypeEnv& env) = 0;

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    Span span;
};

struct FunDef : public Def {
    FunDef(Span span, Name name, std::vector<Param>&& params, type::Type* codomain, Expr* body);

    std::vector<type::Type*> domain() const;

    virtual void declare(TypeEnv& env) override;
    virtual fast::Def* check(fast::Program& program, TypeEnv& env) override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    Name name;
    std::vector<Param> params;
    type::Type* codomain;
    Expr* body;
};

// # Program

struct Program {
    explicit Program(std::vector<Def*>&& defs);

    fast::Program check(type::Types& types);

    void print(Names const& names, std::ostream& dest) const;

    std::vector<Def*> defs;
};

} // namespace brmh::ast

#endif // BRMH_AST_HPP
