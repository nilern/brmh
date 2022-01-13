#ifndef BRMH_FAST_HPP
#define BRMH_FAST_HPP

#include "bumparena.hpp"
#include "type.hpp"
#include "cps/cps.hpp"

namespace brmh::fast {

struct Call;
struct Program;

// # to_cps utils

struct ToCpsCont {
    virtual bool is_trivial() const = 0;
    virtual cps::Expr* operator()(cps::Builder& builder, Span span, cps::Expr*) const = 0;
    virtual cps::Expr* call_to(cps::Builder& builder, Call const* fast_call, std::span<cps::Expr*> cps_exprs) const = 0;
};

struct ToCpsNextCont : public ToCpsCont {
    ToCpsNextCont();

    virtual bool is_trivial() const override;
    virtual cps::Expr* operator()(cps::Builder& builder, Span span, cps::Expr*) const override;
    virtual cps::Expr* call_to(cps::Builder& builder, Call const* fast_call, std::span<cps::Expr*> cps_exprs) const override;
};

struct ToCpsTrivialCont : public ToCpsCont {
    cps::Cont* cont;

    ToCpsTrivialCont(cps::Cont* cont);

    virtual bool is_trivial() const override;
    virtual cps::Expr* operator()(cps::Builder& builder, Span span, cps::Expr*) const override;
    virtual cps::Expr* call_to(cps::Builder& builder, Call const* fast_call, std::span<cps::Expr*> cps_exprs) const override;
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

    virtual cps::Expr* to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const = 0;

    Span span;
    type::Type* type;

protected:
    Expr(Span span, type::Type* type);
};

struct If : public Expr {
    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual cps::Expr* to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const override;

    Expr* cond;
    Expr* conseq;
    Expr* alt;

private:
    friend struct Program;

    If(Span span, type::Type* type, Expr* cond, Expr* conseq, Expr* alt);
};

struct Call : public Expr {
    Expr* callee;
    std::span<Expr*> args;

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

    virtual cps::Expr* to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const override;

private:
    friend struct Program;

    Call(Span span, type::Type* type, Expr* callee_, std::span<Expr*> args_)
        : Expr(span, type), callee(callee_), args(args_) {}
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

    virtual cps::Expr* to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const override;

private:
    friend struct Program;

    AddWI64(Span span, type::Type* type, std::array<Expr*, 2> args);
};

struct SubWI64 : public PrimApp<2> {
    virtual char const* opname() const override;

    virtual cps::Expr* to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const override;

private:
    friend struct Program;

    SubWI64(Span span, type::Type* type, std::array<Expr*, 2> args);
};

struct MulWI64 : public PrimApp<2> {
    virtual char const* opname() const override;

    virtual cps::Expr* to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const override;

private:
    friend struct Program;

    MulWI64(Span span, type::Type* type, std::array<Expr*, 2> args);
};

struct EqI64 : public PrimApp<2> {
    virtual char const* opname() const override { return "eqI64"; }

    virtual cps::Expr* to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const override;

private:
    friend struct Program;

    EqI64(Span span, type::Type* type, std::array<Expr*, 2> args) : PrimApp(span, type, args) {}
};

struct Id : public Expr {
    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual cps::Expr* to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const override;

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

    virtual cps::Expr* to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const override;

    bool value;

private:
    friend struct Program;

    Bool(Span span, type::Type* type, bool v) : Const(span, type), value(v) {}
};

struct I64 : public Const {
    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual cps::Expr* to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const override;

    const char* digits;

private:
    friend struct Program;

    I64(Span span, type::Type* type, const char* digits, std::size_t size);
};

// # Defs

struct Def {
    virtual void print(Names const& names, std::ostream& dest) const = 0;

    virtual void cps_declare(cps::Builder& builder) const = 0;
    virtual void to_cps(cps::Builder& builder) const = 0;

    Span span;

protected:
    Def(Span span);
};

struct FunDef : public Def {
    std::vector<type::Type*> domain() const;

    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual void cps_declare(cps::Builder& builder) const override;
    virtual void to_cps(cps::Builder& builder) const override;

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

    std::span<Expr*> args(std::size_t arity) {
        return std::span<Expr*>(static_cast<Expr**>(arena_.alloc_array<Expr*>(arity)), arity);
    }

    Call* call(Span span, type::Type* type, Expr* callee, std::span<Expr*> args) {
        return new (arena_.alloc<Call>()) Call(span, type, callee, args);
    }

    AddWI64* add_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);
    SubWI64* sub_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);
    MulWI64* mul_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);

    EqI64* eq_i64(Span span, type::Type* type, std::array<Expr*, 2> args) {
        return new(arena_.alloc<EqI64>()) EqI64(span, type, args);
    }

    Id* id(Span span, type::Type* type, Name name);
    Bool* const_bool(Span span, type::Type* type, bool value);
    I64* const_i64(Span span, type::Type* type, const char* chars, std::size_t size);

    FunDef* fun_def(Span span, Name name, std::vector<Param>&& params, type::Type* codomain, Expr* body);

    void push_toplevel(Def* def);

    void print(Names const& names, std::ostream& dest) const;

    cps::Program to_cps(Names& names, type::Types& types) const;

    std::vector<Def*> defs;

private:
    BumpArena arena_;
};

} // namespace brmh

#endif // BRMH_FAST_HPP
