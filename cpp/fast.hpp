#ifndef BRMH_FAST_HPP
#define BRMH_FAST_HPP

#include "bumparena.hpp"
#include "type.hpp"

namespace brmh::fast {

struct Program;

// # Param

struct Param {
    void print(Names const& names, std::ostream& dest) const;

    Name name;
    type::Type* type;

private:
    friend struct Program;

    Param(Name name, type::Type* type);
};

// # Exprs

struct Expr {
    virtual void print(Names const& names, std::ostream& dest) const = 0;

    Span span;
    type::Type* type;

protected:
    Expr(Span span, type::Type* type);
};

struct Id : public Expr {
    virtual void print(Names const& names, std::ostream& dest) const override;

    Name name;

private:
    friend struct Program;

    Id(Span span, type::Type* type, Name name);
};

struct Const : public Expr {
protected:
    Const(Span span, type::Type* type);
};

struct Int : public Const {
    virtual void print(Names const& names, std::ostream& dest) const override;

    const char* digits;

private:
    friend struct Program;

    Int(Span span, type::Type* type, const char* digits, std::size_t size);
};

// # Defs

struct Def {
    virtual void print(Names const& names, std::ostream& dest) const = 0;

    Span span;

protected:
    Def(Span span);
};

struct FunDef : public Def {
    virtual void print(Names const& names, std::ostream& dest) const override;

    Name name;
    std::vector<Param> params;
    type::Type* codomain;
    Expr* body;

private:
    friend struct Program;

    FunDef(Span span, Name name, std::vector<Param>&& params, type::Type* codomain, Expr* body);
};

// # Program

struct Program {
    Param param(Name name, type::Type* type);

    Id* id(Span span, type::Type* type, Name name);
    Int* const_int(Span span, type::Type* type, const char* chars, std::size_t size);

    FunDef* fun_def(Span span, Name name, std::vector<Param>&& params, type::Type* codomain, Expr* body);

    void push_toplevel(Def* def);

    void print(Names const& names, std::ostream& dest) const;

    std::vector<Def*> defs;

private:
    BumpArena arena_;
};

} // namespace brmh

#endif // BRMH_FAST_HPP
