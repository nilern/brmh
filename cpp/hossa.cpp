#include "hossa.hpp"
#include "hossa_schedule.hpp"

#include <cstring>

namespace brmh::hossa {

Fn::Fn(Span span, Name name, type::FnType* type, Block* entry_)
    : Expr(span, name, type), entry(entry_) {}

void Fn::print_def(Names& names, std::ostream& dest) const {
    schedule::Schedule schedule = schedule::schedule_late(this);

    std::unordered_set<Block const*> visited_blocks;
    std::unordered_set<Expr const*> visited_exprs;

    dest << "fun ";
    name.print(names, dest);

    dest << " -> ";

    static_cast<type::FnType*>(type)->codomain->print(names, dest); // HACK: static_cast

    dest << " {" << std::endl;

    dest << "    ";
    entry->print(names, dest, visited_blocks, visited_exprs);

    dest << std::endl << '}';
}

void Fn::do_print(Names &names, std::ostream &dest) const {
    dest << "fun ";
    name.print(names, dest);
}

Block::Block(Name name_, std::span<Param*> params_, Transfer* transfer_)
    : name(name_), params(params_), transfer(transfer_) {}

void Block::print(Names& names, std::ostream& dest, std::unordered_set<Block const*>& visited_blocks,
                  std::unordered_set<Expr const*>& visited_exprs) const {
    if (!visited_blocks.contains(this)) {
        visited_blocks.insert(this);
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

        dest << "):" << std::endl;

        transfer->print(names, dest, visited_blocks, visited_exprs);
    }
}

Expr::Expr(Span span_, Name name_, type::Type* type_)
    : span(span_), name(name_), type(type_) {}

AddWI64::AddWI64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args)
    : PrimApp(span, name, type, args) {}

char const* AddWI64::opname() const { return  "addWI64"; }

SubWI64::SubWI64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args)
    : PrimApp(span, name, type, args) {}

char const* SubWI64::opname() const { return "subWI64"; }

MulWI64::MulWI64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args)
    : PrimApp(span, name, type, args) {}

char const* MulWI64::opname() const { return "mulWI64"; }

Param::Param(Span span, Name name, type::Type* type)
    : Expr(span, name, type){}

void Param::do_print(Names& names, std::ostream& dest) const {
    dest << "param ";
    name.print(names, dest);
}

I64::I64(Span span, Name name, type::Type* type, const char* digits_)
    : Expr(span, name, type), digits(digits_) {}

void I64::do_print(Names&, std::ostream& dest) const {
    dest << digits;
}

Transfer::Transfer(Span span_) : span(span_) {}

void Transfer::print(Names& names, std::ostream& dest, std::unordered_set<Block const*>& visited_blocks,
           std::unordered_set<Expr const*>& visited_exprs) const {
    for (Expr const* operand : operands()) {
        operand->print(names, dest, visited_exprs);
    }

    do_print(names, dest);

    for (Block const* succ : successors()) {
        dest << "\n\n    ";
        succ->print(names, dest, visited_blocks, visited_exprs);
    }
}

If::If(Span span, Expr *cond_, Block *conseq_, Block *alt_)
    : Transfer(span), cond(cond_), conseq(conseq_), alt(alt_) {}

void If::do_print(Names& names, std::ostream& dest) const {
    dest << "        if ";
    cond->name.print(names, dest);
    dest << "\n        then goto ";
    conseq->name.print(names, dest);
    dest << "\n        else goto ";
    alt->name.print(names, dest);
}

Return::Return(Span span, Expr* res_) : Transfer(span), res(res_) {}

void Return::do_print(Names& names, std::ostream& dest) const {
    dest << "        return ";
    res->name.print(names, dest);
}

Goto::Goto(Span span, Block* dest_, Expr* res_) : Transfer(span), dest(dest_), res(res_) {}

void Goto::do_print(Names& names, std::ostream& desto) const {
    desto << "        goto ";
    dest->name.print(names, desto);
    desto << ' ';
    res->name.print(names, desto);
}

// # Program

Program::Program(BumpArena&& arena, std::vector<Fn*> &&externs_)
    : externs(std::move(externs_)), arena_(std::move(arena)) {}

void Program::print(Names& names, std::ostream& dest) const {
    for (Fn* const ext_fn : externs) {
        ext_fn->print_def(names, dest);
        dest << std::endl << std::endl;
    }
}

// # Builder

Builder::Builder(Names* names, type::Types& types)
    : names_(names), types_(types), arena_(),
      exprs_(), externs_(),
      current_block_(opt_ptr<Block>::none()) {}

Names *Builder::names() const { return names_; }

Fn* Builder::fn(Span span, Name name, type::FnType* type, bool external, Block* entry) {
    Fn* const res = new (arena_.alloc<Fn>()) Fn(span, name, type, entry);
    exprs_.insert({name, res});
    if (external) { externs_.push_back(res); }
    return res;
}

opt_ptr<Block> Builder::current_block() const { return current_block_; }

void Builder::set_current_block(Block* block) { current_block_ = opt_ptr<Block>::some(block); }

Block* Builder::block(std::size_t arity, Transfer* transfer) {
    std::span<Param*> const params = std::span(static_cast<Param**>(arena_.alloc_array<Param*>(arity)), arity);
    return new (arena_.alloc<Block>()) Block(names_->fresh(), params, transfer);
}

Param* Builder::param(Span span, type::Type* type, Block* block_, Name name, std::size_t index) {
    Param* const param = new (arena_.alloc<Param>()) Param(span, name, type);

    block_->params[index] = param;

    exprs_.insert({name, param});

    return param;
}

Bool *Builder::const_bool(Span span, type::Type *type, bool value) {
    return new (arena_.alloc<Bool>()) Bool(span, names_->fresh(), type, value);
}

I64* Builder::const_i64(Span span, type::Type* type, const char* digits_, std::size_t size) {
    char* digits = static_cast<char*>(arena_.alloc_array<char>(size));
    strncpy(digits, digits_, size);
    return new (arena_.alloc<I64>()) I64(span, names_->fresh(), type, digits);
}

Transfer *Builder::if_(Span span, Expr *cond, Block *conseq, Block *alt) {
    return new (arena_.alloc<If>()) If(span, cond, conseq, alt);
}

Transfer *Builder::goto_(Span span, Block *dest, Expr *res) { return new (arena_.alloc<Goto>()) Goto(span, dest, res); }

Transfer* Builder::ret(Span span, Expr* expr) { return new (arena_.alloc<Return>()) Return(span, expr); }

Expr* Builder::add_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args) {
    return new (arena_.alloc<AddWI64>()) AddWI64(span, names_->fresh(), type, args);
}

Expr* Builder::sub_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args) {
    return new (arena_.alloc<SubWI64>()) SubWI64(span, names_->fresh(), type, args);
}

Expr* Builder::mul_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args) {
    return new (arena_.alloc<MulWI64>()) MulWI64(span, names_->fresh(), type, args);
}

Expr* Builder::id(Name name) { return exprs_.at(name); }

Program Builder::build() { return Program(std::move(arena_), std::move(externs_)); }

} // namespace brmh::hossa
