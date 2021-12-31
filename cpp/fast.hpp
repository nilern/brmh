#ifndef BRMH_FAST_HPP
#define BRMH_FAST_HPP

#include "bumparena.hpp"
#include "type.hpp"
#include "hossa.hpp"

namespace brmh::fast {

struct Program;

// # to_hossa utils

struct ToHossaCont {
    virtual bool is_trivial() const = 0;
    virtual hossa::Expr* operator()(hossa::Builder& builder, Span span, hossa::Expr*) const = 0;
};

struct ToHossaNextCont : public ToHossaCont {
    ToHossaNextCont();

    virtual bool is_trivial() const override;
    virtual hossa::Expr* operator()(hossa::Builder& builder, Span span, hossa::Expr*) const override;
};

struct ToHossaLabelCont : public ToHossaCont {
    ToHossaLabelCont(hossa::Block*);

    virtual bool is_trivial() const override;
    virtual hossa::Expr* operator()(hossa::Builder& builder, Span span, hossa::Expr*) const override;

    hossa::Block* block;
};

struct ToHossaReturnCont : public ToHossaCont {
    ToHossaReturnCont();

    virtual bool is_trivial() const override;
    virtual hossa::Expr* operator()(hossa::Builder& builder, Span span, hossa::Expr*) const override;
};

// # Param

struct Param {
    void print(Names const& names, std::ostream& dest) const;

    Span span;
    Name name;
    type::Type* type;

private:
    friend struct Program;

    Param(Span span, Name name, type::Type* type);
};

// # Exprs

struct Expr {
    virtual void print(Names const& names, std::ostream& dest) const = 0;

    virtual hossa::Expr* to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const = 0;

    Span span;
    type::Type* type;

protected:
    Expr(Span span, type::Type* type);
};

struct If : public Expr {
    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual hossa::Expr* to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const override;

    Expr* cond;
    Expr* conseq;
    Expr* alt;

private:
    friend struct Program;

    If(Span span, type::Type* type, Expr* cond, Expr* conseq, Expr* alt);
};

template<std::size_t N>
struct PrimApp : public Expr {
    virtual void print(Names const& names, std::ostream& dest) const override {
        dest << "__" << opname();

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

    virtual char const* opname() const = 0;

    std::array<Expr*, N> args;

protected:
    PrimApp(Span span, type::Type* type, std::array<Expr*, N> args_) : Expr(span, type), args(args_) {}
};

struct AddWI64 : public PrimApp<2> {
    virtual char const* opname() const override;

    virtual hossa::Expr* to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const override;

private:
    friend struct Program;

    AddWI64(Span span, type::Type* type, std::array<Expr*, 2> args);
};

struct SubWI64 : public PrimApp<2> {
    virtual char const* opname() const override;

    virtual hossa::Expr* to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const override;

private:
    friend struct Program;

    SubWI64(Span span, type::Type* type, std::array<Expr*, 2> args);
};

struct MulWI64 : public PrimApp<2> {
    virtual char const* opname() const override;

    virtual hossa::Expr* to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const override;

private:
    friend struct Program;

    MulWI64(Span span, type::Type* type, std::array<Expr*, 2> args);
};

struct Id : public Expr {
    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual hossa::Expr* to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const override;

    Name name;

private:
    friend struct Program;

    Id(Span span, type::Type* type, Name name);
};

struct Const : public Expr {
protected:
    Const(Span span, type::Type* type);
};

struct Bool : public Const {
    virtual void print(Names const&, std::ostream& dest) const override {
        dest << (value ? "True" : "False");
    }

    virtual hossa::Expr* to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const override;

    bool value;

private:
    friend struct Program;

    Bool(Span span, type::Type* type, bool v) : Const(span, type), value(v) {}
};

struct I64 : public Const {
    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual hossa::Expr* to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const override;

    const char* digits;

private:
    friend struct Program;

    I64(Span span, type::Type* type, const char* digits, std::size_t size);
};

// # Defs

struct Def {
    virtual void print(Names const& names, std::ostream& dest) const = 0;

    virtual void to_hossa(hossa::Builder& builder) const = 0;

    Span span;

protected:
    Def(Span span);
};

struct FunDef : public Def {
    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual void to_hossa(hossa::Builder& builder) const override;

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
    Param param(Span span, Name name, type::Type* type);

    If* if_(Span span, type::Type* type, Expr* cond, Expr* conseq, Expr* alt);
    AddWI64* add_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);
    SubWI64* sub_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);
    MulWI64* mul_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);
    Id* id(Span span, type::Type* type, Name name);
    Bool* const_bool(Span span, type::Type* type, bool value);
    I64* const_i64(Span span, type::Type* type, const char* chars, std::size_t size);

    FunDef* fun_def(Span span, Name name, std::vector<Param>&& params, type::Type* codomain, Expr* body);

    void push_toplevel(Def* def);

    void print(Names const& names, std::ostream& dest) const;

    hossa::Program to_hossa(Names& names) const;

    std::vector<Def*> defs;

private:
    BumpArena arena_;
};

} // namespace brmh

#endif // BRMH_FAST_HPP
