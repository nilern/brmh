#ifndef BRMH_AST_HPP
#define BRMH_AST_HPP

#include <vector>

#include "span.hpp"
#include "name.hpp"
#include "type.hpp"

namespace brmh::ast {

// # Param

struct Param {
    Name name;
    type::Type* type;

    void print(Names const& names, std::ostream& dest) const;
};

// # Exprs

struct Expr {
    explicit Expr(Span span);

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    Span span;
};

struct Id : public Expr {
    Id(Span pos, Name name);

    virtual void print(Names const& names, std::ostream& dest) const override;

    Name name;
};

struct Const : public Expr {
    explicit Const(Span span);
};

struct Int : public Const {
    Int(Span pos, const char* chars, std::size_t size);

    virtual void print(Names const& names, std::ostream& dest) const override;

    const char* digits;
};

// # Defs

struct Def {
    explicit Def(Span span);

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    Span span;
};

struct FunDef : public Def {
    FunDef(Span span, Name name, std::vector<Param>&& params, type::Type* codomain, Expr* body);

    virtual void print(Names const& names, std::ostream& dest) const override;

    Name name;
    std::vector<Param> params;
    type::Type* codomain;
    Expr* body;
};

// # Program

struct Program {
    explicit Program(std::vector<Def*>&& defs);

    void print(Names const& names, std::ostream& dest) const;

    std::vector<Def*> defs;
};

} // namespace brmh::ast

#endif // BRMH_AST_HPP
