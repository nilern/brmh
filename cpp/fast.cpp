#include "fast.hpp"

#include <cstring>

namespace brmh::fast {

// # Param

Param::Param(Span span_, Name name_, type::Type* type_) : span(span_), name(name_), type(type_) {}

void Param::print(Names const& names, std::ostream& dest) const {
    name.print(names, dest);
    dest << " : ";
    type->print(names, dest);
}

// # Expr

Expr::Expr(Span span_, type::Type* type_) : span(span_), type(type_) {}

// ## If

If::If(Span span, type::Type* type, Expr* cond_, Expr* conseq_, Expr* alt_)
    : Expr(span, type), cond(cond_), conseq(conseq_), alt(alt_) {}

void If::print(Names const& names, std::ostream& dest) const {
    dest << "if ";
    cond->print(names, dest);
    dest << "}\n    ";
    conseq->print(names, dest);
    dest << "\n} else {\n    ";
    alt->print(names, dest);
    dest << "\n}\n";
}

// # PrimApp

AddWI64::AddWI64(Span span, type::Type *type, std::array<Expr *, 2> args) : PrimApp<2>(span, type, args) {}

char const* AddWI64::opname() const { return "addWI64"; }

SubWI64::SubWI64(Span span, type::Type *type, std::array<Expr *, 2> args) : PrimApp<2>(span, type, args) {}

char const*  SubWI64::opname() const { return "subWI64"; }

MulWI64::MulWI64(Span span, type::Type *type, std::array<Expr *, 2> args) : PrimApp<2>(span, type, args) {}

char const*  MulWI64::opname() const { return "mulWI64"; }

// ## Id

Id::Id(Span span, type::Type* type, Name name_) : Expr(span, type), name(name_) {}

void Id::print(Names const& names, std::ostream& dest) const { name.print(names, dest); }

// ## Const

Const::Const(Span span, type::Type* type) : Expr(span, type) {}

// ### I64

I64::I64(Span span, type::Type* type, const char* digits, std::size_t size)
    : Const(span, type), digits(strndup(digits, size)) {}

void I64::print(Names const&, std::ostream& dest) const { dest << digits; }

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

Param Program::param(Span span, Name name, type::Type* type) { return Param(span, name, type); }

If* Program::if_(Span span, type::Type *type, Expr *cond, Expr *conseq, Expr *alt) {
    return new(arena_.alloc<If>()) If(span, type, cond, conseq, alt);
}

AddWI64* Program::add_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args){
    return new(arena_.alloc<AddWI64>()) AddWI64(span, type, args);
}

SubWI64* Program::sub_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args){
    return new(arena_.alloc<SubWI64>()) SubWI64(span, type, args);
}

MulWI64* Program::mul_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args){
    return new(arena_.alloc<MulWI64>()) MulWI64(span, type, args);
}

Id* Program::id(Span span, type::Type* type, Name name) {
    return new(arena_.alloc<Id>()) Id(span, type, name); }

I64* Program::const_i64(Span span, type::Type* type, const char* chars, std::size_t size) {
    return new(arena_.alloc<I64>()) I64(span, type, chars, size);
}

FunDef* Program::fun_def(Span span, Name name, std::vector<Param>&& params, type::Type* codomain, Expr* body) {
    return new(arena_.alloc<FunDef>()) FunDef(span, name, std::move(params), codomain, body);
}

void Program::push_toplevel(Def* def) {
    defs.push_back(def);
}

void Program::print(Names const& names, std::ostream& dest) const {
    for (const auto def : defs) {
        def->print(names, dest);
        dest << std::endl << std::endl;
    }
}

} // namespace brmh
