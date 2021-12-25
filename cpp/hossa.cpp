#include "hossa.hpp"

#include <cstring>

namespace brmh::hossa {

Fn::Fn(Span span_, Name name_, type::Type* codomain_, Block* entry_)
    : span(span_), name(name_), codomain(codomain_), entry(entry_) {}

void Fn::print(Names& names, std::ostream& dest) const {
    dest << "fun ";
    name.print(names, dest);

    dest << " -> ";

    codomain->print(names, dest);

    dest << " {" << std::endl;

    dest << "    ";
    entry->print(names, dest);

    dest << std::endl << '}';
}

Block::Block(Fn* fn_, Name name_, std::span<Param*> params_, Transfer* transfer_)
    : fn(fn_), name(name_), params(params_), transfer(transfer_) {}

void Block::print(Names& names, std::ostream& dest) const {
    name.print(names, dest);

    dest << " (";

    auto param = params.begin();
    if (param != params.end()) {
        (*param)->name.print(names, dest);
        dest << " : ";
        (*param)->type->print(names, dest);
        ++param;

        for (; param != params.end(); ++param) {
            dest << ", ";
            (*param)->name.print(names, dest);
            dest << " : ";
            (*param)->type->print(names, dest);
        }
    }

    dest << ") {" << std::endl;

    transfer->print(names, dest);
    dest << ';';

    dest << std::endl << "    }";
}

Expr::Expr(opt_ptr<Block> block_, Span span_, Name name_, type::Type* type_)
    : block(block_), span(span_), name(name_), type(type_) {}

Param::Param(opt_ptr<Block> block, Span span, Name name, type::Type* type)
    : Expr(block, span, name, type){}

void Param::print(Names& names, std::ostream& dest) const {
    name.print(names, dest);
}

I64::I64(opt_ptr<Block> block, Span span, Name name, type::Type* type, const char* digits_)
    : Expr(block, span, name, type), digits(digits_) {}

void I64::print(Names&, std::ostream& dest) const {
    dest << digits;
}

Transfer::Transfer(Span span_) : span(span_) {}

Return::Return(Span span, Expr* res_) : Transfer(span), res(res_) {}

void Return::print(Names& names, std::ostream& dest) const {
    dest << "        return ";
    res->print(names, dest);
}

// # Program

Program::Program(BumpArena&& arena, std::vector<Fn*> &&externs_)
    : externs(std::move(externs_)), arena_(std::move(arena)) {}

void Program::print(Names& names, std::ostream& dest) const {
    for (Fn* const ext_fn : externs) {
        ext_fn->print(names, dest);
        dest << std::endl << std::endl;
    }
}

// # Builder

Builder::Builder(Names* names) : names_(names), arena_(), exprs_(), externs_() {}

Fn* Builder::fn(Span span, Name name, type::Type* codomain, bool external, Block* entry) {
    Fn* const res = new (arena_.alloc<Fn>()) Fn(span, name, codomain, entry);
    if (external) { externs_.push_back(res); }
    return res;
}

Block* Builder::block(Fn* fn, std::size_t arity, Transfer* transfer) {
    std::span<Param*> const params = std::span(static_cast<Param**>(arena_.alloc_array<Param*>(arity)), arity);
    return new (arena_.alloc<Block>()) Block(fn, names_->fresh(), params, transfer);
}

Param* Builder::param(Span span, type::Type* type, Block* block_, Name name, std::size_t index) {
    Param* const param = new (arena_.alloc<Param>()) Param(opt_ptr<Block>::some(block_), span, name, type);

    block_->params[index] = param;

    exprs_.insert({name, param});

    return param;
}

I64* Builder::const_i64(Span span, type::Type* type, const char* digits_, std::size_t size) {
    char* digits = static_cast<char*>(arena_.alloc_array<char>(size));
    strncpy(digits, digits_, size);
    return new (arena_.alloc<I64>()) I64(opt_ptr<Block>::none(), span, names_->fresh(), type, digits);
}

Transfer* Builder::ret(Span span, Expr* expr) { return new (arena_.alloc<Return>()) Return(span, expr); }

Expr* Builder::id(Name name) { return exprs_.at(name); }

Program Builder::build() { return Program(std::move(arena_), std::move(externs_)); }

} // namespace brmh::hossa
