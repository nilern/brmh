#include "fast.hpp"

#include <cstring>

namespace brmh::fast {

// # Param

Param::Param(Name name_, type::Type* type_) : name(name_), type(type_) {}

void Param::print(Names const& names, std::ostream& dest) const {
    name.print(names, dest);
    dest << " : ";
    type->print(names, dest);
}

// # Expr

Expr::Expr(Span span_, type::Type* type_) : span(span_), type(type_) {}

// ## Id

Id::Id(Span span, type::Type* type, Name name_) : Expr(span, type), name(name_) {}

void Id::print(Names const& names, std::ostream& dest) const { name.print(names, dest); }

// ## Const

Const::Const(Span span, type::Type* type) : Expr(span, type) {}

// ### Int

Int::Int(Span span, type::Type* type, const char* digits, std::size_t size)
    : Const(span, type), digits(strndup(digits, size)) {}

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

Param Program::param(Name name, type::Type* type) { return Param(name, type); }

Id* Program::id(Span span, type::Type* type, Name name) {
    return new(arena_.alloc(sizeof(Id))) Id(span, type, name); }

Int* Program::const_int(Span span, type::Type* type, const char* chars, std::size_t size) {
    return new(arena_.alloc(sizeof(Int))) Int(span, type, chars, size);
}

FunDef* Program::fun_def(Span span, Name name, std::vector<Param>&& params, type::Type* codomain, Expr* body) {
    return new(arena_.alloc(sizeof(FunDef))) FunDef(span, name, std::move(params), codomain, body);
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
