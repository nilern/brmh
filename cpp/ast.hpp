#ifndef BRMH_AST_HPP
#define BRMH_AST_HPP

#include <vector>

#include "span.hpp"
#include "name.hpp"
#include "type.hpp"
#include "typeenv.hpp"

namespace brmh::ast {

// # Param

struct Param {
    Name name;
    type::Type* type;

    void declare(TypeEnv& env) const;

    void print(Names const& names, std::ostream& dest) const;
};

// # Exprs

struct Expr {
    explicit Expr(Span span);

    virtual std::pair<Expr*, type::Type*> type_of(TypeEnv& env) const = 0;
    Expr* check(TypeEnv& env, type::Type* type) const;

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    Span span;
};

struct Id : public Expr {
    Id(Span pos, Name name);

    virtual std::pair<Expr*, type::Type*> type_of(TypeEnv& env) const override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    Name name;
};

struct Const : public Expr {
    explicit Const(Span span);
};

struct Int : public Const {
    Int(Span pos, const char* chars, std::size_t size);

    virtual std::pair<Expr*, type::Type*> type_of(TypeEnv& env) const override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    const char* digits;
};

// # Defs

struct Def {
    explicit Def(Span span);

    virtual void declare(TypeEnv& env) = 0;
    virtual Def* check(TypeEnv& env) = 0;

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    Span span;
};

struct FunDef : public Def {
    FunDef(Span span, Name name, std::vector<Param>&& params, type::Type* codomain, Expr* body);

    std::vector<type::Type*> domain() const;

    virtual void declare(TypeEnv& env) override;
    virtual Def* check(TypeEnv& env) override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    Name name;
    std::vector<Param> params;
    type::Type* codomain;
    Expr* body;
};

// # Program

struct Program {
    explicit Program(std::vector<Def*>&& defs);

    Program check(type::Types& types);

    void print(Names const& names, std::ostream& dest) const;

    std::vector<Def*> defs;
};

} // namespace brmh::ast

#endif // BRMH_AST_HPP
