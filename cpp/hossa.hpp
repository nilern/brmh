#ifndef HOSSA_HPP
#define HOSSA_HPP

#include <span>
#include <unordered_set>
#include <ostream>

#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include "util.hpp"
#include "bumparena.hpp"
#include "name.hpp"
#include "type.hpp"

namespace brmh {

struct ToLLVMCtx;

}

namespace brmh::hossa {

struct Expr;
struct Transfer;
struct Block;
struct Fn;
class Builder;

// # Visitors

struct TransfersExprsVisitor {
    virtual void visit(Transfer const* transfer) = 0;
    virtual void visit(Expr const* transfer) = 0;
};

// # Expr

struct Expr {
    Span span;
    Name name;
    type::Type* type;

protected:
    Expr(Span span_, Name name_, type::Type* type_)
        : span(span_), name(name_), type(type_) {}

public:
    virtual std::span<Expr* const> operands() const = 0;

    template<typename F>
    void do_post_visit(std::unordered_set<Expr const*>& visited, F f) const {
        if (!visited.contains(this)) {
            visited.insert(this);

            for (Expr const* operand : operands()) {
                operand->do_post_visit(visited, f);
            }

            f(this);
        }
    }

    virtual void do_print(Names& names, std::ostream& dest) const = 0;

    void print_in(Names& names, std::ostream& dest, std::unordered_map<Expr const*, Block const*> const& schedule,
                  std::unordered_set<Expr const*>& visited_exprs, Block const* block) const;

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const = 0;
    llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const;
};

// ## Call

struct Call : public Expr {
    std::span<Expr*> exprs;

private:
    friend class Builder;

    Call(Span span, Name name, type::Type* type, std::span<Expr*> exprs_)
        : Expr(span, name, type), exprs(exprs_) {}

public:
    Expr* callee() const { return exprs[0]; }

    std::span<Expr* const> args() const { return exprs.subspan(1); }

    virtual std::span<Expr* const> operands() const override { return exprs; }

    virtual void do_print(Names& names, std::ostream& dest) const override {
        callee()->name.print(names, dest);

        dest << '(';

        auto arg = args().begin();
        if (arg != args().end()) {
            (*arg)->name.print(names, dest);
            ++arg;

            for (; arg != args().end(); ++arg) {
                dest << ", ";
                (*arg)->name.print(names, dest);
            }
        }

        dest << ')';
    }

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

// ## PrimApp

template<std::size_t N>
struct PrimApp : public Expr {
    std::array<Expr*, N> args;

protected:
    PrimApp(Span span, Name name, type::Type* type, std::array<Expr*, N> args_)
        : Expr(span, name, type), args(args_) {}

public:
    virtual const char* opname() const = 0;

    virtual std::span<Expr* const> operands() const override {
        return std::span<Expr* const>(args);
    }

    virtual void do_print(Names& names, std::ostream& dest) const override {
        dest << "__" << opname();

        dest << '(';

        auto arg = args.begin();
        if (arg != args.end()) {
            (*arg)->name.print(names, dest);
            ++arg;

            for (; arg != args.end(); ++arg) {
                dest << ", ";
                (*arg)->name.print(names, dest);
            }
        }

        dest << ')';
    }
};

struct AddWI64 : public PrimApp<2> {
private:
    friend class Builder;

    AddWI64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args)
        : PrimApp(span, name, type, args) {}

public:
    char const* opname() const override { return  "addWI64"; }

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

struct SubWI64 : public PrimApp<2> {
private:
    friend class Builder;

    SubWI64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args)
        : PrimApp(span, name, type, args) {}

public:
    virtual const char* opname() const override { return "subWI64"; }

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

struct MulWI64 : public PrimApp<2> {
private:
    friend class Builder;

    MulWI64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args)
        : PrimApp(span, name, type, args) {}

public:
    virtual const char* opname() const override { return "mulWI64"; }

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

struct EqI64 : public PrimApp<2> {
private:
    friend class Builder;

    EqI64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args) : PrimApp(span, name, type, args) {}

public:
    virtual const char* opname() const override { return "eqI64"; }

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

// ## Param

struct Param : public Expr {
private:
    friend class Builder;

    Param(Span span, Name name, type::Type* type) : Expr(span, name, type){}

public:
    virtual std::span<Expr* const> operands() const override {
        return std::span<Expr* const>();
    }

    void do_print(Names& names, std::ostream& dest) const override {
        dest << "param ";
        name.print(names, dest);
    }

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

// ## Const

struct Const : public Expr {
    virtual std::span<Expr* const> operands() const override { return std::span<Expr* const>(); }

protected:
    Const(Span span, Name name, type::Type* type) : Expr(span, name, type) {}
};

// ## I64

struct I64 : public Const {
    const char* digits;

private:
    friend class Builder;

    I64(Span span, Name name, type::Type* type, const char* digits_)
        : Const(span, name, type), digits(digits_) {}

public:
    void do_print(Names&, std::ostream& dest) const override {
        dest << digits;
    }

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

// ## Bool

struct Bool : public Const {
    bool value;

private:
    friend class Builder;

    Bool(Span span, Name name, type::Type* type, bool v) : Const(span, name, type), value(v) {}

public:
    virtual void do_print(Names&, std::ostream& dest) const override {
        dest << (value ? "True" : "False");
    }

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

// # Transfer

struct Transfer {
    Span span;

protected:
    Transfer(Span span_) : span(span_) {}

public:
    virtual std::span<Expr* const> operands() const = 0;
    virtual std::span<Block* const> successors() const = 0;

    virtual void do_print(Names& names, std::ostream& dest) const = 0;

    void print(Names& names, std::ostream& dest, std::unordered_map<Expr const*, Block const*> const& schedule,
               std::unordered_set<Block const*>& visited_blocks, std::unordered_set<Expr const*>& visited_exprs,
               Block const* block) const;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const = 0;
};

// ## If

struct If : public Transfer {
    Expr* cond;
    Block* conseq;
    Block* alt;

private:
    friend class Builder;

    If(Span span, Expr *cond_, Block *conseq_, Block *alt_)
        : Transfer(span), cond(cond_), conseq(conseq_), alt(alt_) {}

public:
    virtual std::span<Expr* const> operands() const override {
        return std::span<Expr* const>(&cond, 1);
    }

    virtual std::span<Block* const> successors() const override {
        return std::span<Block* const>(&conseq, 2);
    }

    void do_print(Names& names, std::ostream& dest) const override;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

// ## TailCall

struct TailCall : public Transfer {
    std::span<Expr*> exprs;

private:
    friend class Builder;

    TailCall(Span span, std::span<Expr*> exprs_) : Transfer(span), exprs(exprs_) {}

public:
    Expr* callee() const { return exprs[0]; }

    std::span<Expr* const> args() const { return exprs.subspan(1); }

    virtual std::span<Expr* const> operands() const override { return exprs; }

    virtual std::span<Block* const> successors() const override {
        return std::span<Block* const>();
    }

    void do_print(Names& names, std::ostream& dest) const override {
        dest << "        tailcall ";
        callee()->name.print(names, dest);

        dest << '(';

        auto arg = args().begin();
        if (arg != args().end()) {
            (*arg)->name.print(names, dest);
            ++arg;

            for (; arg != args().end(); ++arg) {
                dest << ", ";
                (*arg)->name.print(names, dest);
            }
        }

        dest << ')';
    }

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

// ## Goto

struct Goto : public Transfer {
    Block* dest;
    Expr* res;

private:
    friend class Builder;

    Goto(Span span, Block* dest_, Expr* res_) : Transfer(span), dest(dest_), res(res_) {}

public:
    virtual std::span<Expr* const> operands() const override {
        return std::span<Expr* const>(&res, 1);
    }

    virtual std::span<Block* const> successors() const override {
        return std::span<Block* const>(&dest, 1);
    }

    void do_print(Names& names, std::ostream& desto) const override;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

// ## Return

struct Return : public Transfer {
    Expr* res;

private:
    friend class Builder;

    Return(Span span, Expr* res_) : Transfer(span), res(res_) {}

public:
    virtual std::span<Expr* const> operands() const override {
        return std::span<Expr* const>(&res, 1);
    }

    virtual std::span<Block* const> successors() const override { return std::span<Block* const>(); }

    void do_print(Names& names, std::ostream& dest) const override {
        dest << "        return ";
        res->name.print(names, dest);
    }

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
};

// # Block

struct Block {
    Name name;
    std::span<Param*> params;
    Transfer* transfer;

private:
    friend class Builder;

    Block(Name name_, std::span<Param*> params_, Transfer* transfer_)
        : name(name_), params(params_), transfer(transfer_) {}

public:
    template<typename F>
    void do_post_visit(std::unordered_set<Block const*>& visited, F f) const {
        if (!visited.contains(this)) {
            visited.insert(this);

            for (Block const* succ : transfer->successors()) {
                succ->do_post_visit(visited, f);
            }

            f(this);
        }
    }

    void do_post_visit_transfers_and_exprs(std::unordered_set<Block const*>& visited_blocks,
                                           std::unordered_set<Expr const*>& visited_exprs,
                                           TransfersExprsVisitor& visitor) const {
        do_post_visit(visited_blocks, [&] (Block const* block) {
            for (Expr const* expr : block->transfer->operands()) {
                expr->do_post_visit(visited_exprs, [&] (Expr const* expr) {
                    visitor.visit(expr);
                });
            }

            visitor.visit(block->transfer);
        });
    }

    void print(Names& names, std::ostream& dest, std::unordered_map<Expr const*, Block const*> const& schedule,
               std::unordered_set<Block const*>& visited_blocks,
               std::unordered_set<Expr const*>& visited_exprs) const;

    void llvm_declare(ToLLVMCtx& ctx) const;
    llvm::BasicBlock* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const;
};

// # Fn

struct Fn : public Expr {
    Block* entry;

private:
    friend class Builder;

    Fn(Span span, Name name, type::FnType* type, Block* entry_)
        : Expr(span, name, type), entry(entry_) {}

public:
    virtual std::span<Expr* const> operands() const override { return std::span<Expr* const>(); }

    template<typename F>
    void post_visit_blocks(F f) const {
        std::unordered_set<Block const*> visited;
        entry->do_post_visit(visited, f);
    }

    virtual void post_visit_transfers_and_exprs(TransfersExprsVisitor& visitor) const {
        std::unordered_set<Block const*> visited_blocks;
        std::unordered_set<Expr const*> visited_exprs;
        entry->do_post_visit_transfers_and_exprs(visited_blocks, visited_exprs, visitor);
    }

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

    void do_print(Names &names, std::ostream &dest) const override {
        dest << "fun ";
        name.print(names, dest);
    }

    void print_def(Names& names, std::ostream& dest) const;

    void llvm_declare(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module, llvm::Function::LinkageTypes linkage) const;
    void llvm_define(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module) const;
};

// # Program

struct Program {
    std::vector<Fn*> externs;
private:
    BumpArena arena_;

    friend class Builder;

    Program(BumpArena&& arena, std::vector<Fn*> &&externs_)
        : externs(std::move(externs_)), arena_(std::move(arena)) {}

public:
    void print(Names& names, std::ostream& dest) const {
        for (Fn* const ext_fn : externs) {
            ext_fn->print_def(names, dest);
            dest << std::endl << std::endl;
        }
    }

    void to_llvm(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module) const;
};

// # Builder

class Builder {
    Names* names_;
    type::Types& types_;
    BumpArena arena_;
    std::unordered_map<Name, Expr*, Name::Hash> exprs_;
    std::vector<Fn*> externs_;
    opt_ptr<Block> current_block_;

public:
    Builder(Names* names, type::Types& types)
        : names_(names), types_(types), arena_(),
          exprs_(), externs_(),
          current_block_(opt_ptr<Block>::none()) {}

    Names *names() const { return names_; }

    type::Types& types() const { return types_; }

    opt_ptr<Block> current_block() const { return current_block_; }

    void set_current_block(Block* block) { current_block_ = opt_ptr<Block>::some(block); }

    // OPTIMIZE: deduplicate constants, constant folding, CSE etc.:

    Fn* fn(Span span, Name name, type::FnType* type, bool external, Block* entry) {
        Fn* const res = new (arena_.alloc<Fn>()) Fn(span, name, type, entry);
        exprs_.insert({name, res});
        if (external) { externs_.push_back(res); }
        return res;
    }

    Fn* get_fn(Name name) const { return static_cast<Fn*>(exprs_.at(name)); }

    Block* block(std::size_t arity, Transfer* transfer) {
        std::span<Param*> const params = std::span(static_cast<Param**>(arena_.alloc_array<Param*>(arity)), arity);
        return new (arena_.alloc<Block>()) Block(names_->fresh(), params, transfer);
    }

    Param* param(Span span, type::Type* type, Block* block_, Name name, std::size_t index) {
        Param* const param = new (arena_.alloc<Param>()) Param(span, name, type);

        block_->params[index] = param;

        exprs_.insert({name, param});

        return param;
    }

    std::span<Expr*> args(std::size_t arity) {
        return std::span<Expr*>(static_cast<Expr**>(arena_.alloc_array<Expr*>(arity)), arity);
    }

    Transfer *if_(Span span, Expr *cond, Block *conseq, Block *alt) {
        return new (arena_.alloc<If>()) If(span, cond, conseq, alt);
    }

    Transfer* tail_call(Span span, std::span<Expr*> exprs) {
        return new (arena_.alloc<TailCall>()) TailCall(span, exprs);
    }

    Transfer *goto_(Span span, Block *dest, Expr *res) {
        return new (arena_.alloc<Goto>()) Goto(span, dest, res);
    }

    Transfer* ret(Span span, Expr* expr) {
        return new (arena_.alloc<Return>()) Return(span, expr);
    }

    Expr* call(Span span, type::Type* type, std::span<Expr*> exprs) {
        return new (arena_.alloc<Call>()) Call(span, names_->fresh(), type, exprs);
    }

    Expr* add_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args) {
        return new (arena_.alloc<AddWI64>()) AddWI64(span, names_->fresh(), type, args);
    }

    Expr* sub_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args) {
        return new (arena_.alloc<SubWI64>()) SubWI64(span, names_->fresh(), type, args);
    }

    Expr* mul_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args) {
        return new (arena_.alloc<MulWI64>()) MulWI64(span, names_->fresh(), type, args);
    }

    Expr* eq_i64(Span span, type::Type* type, std::array<Expr*, 2> args) {
        return new (arena_.alloc<EqI64>()) EqI64(span, names_->fresh(), type, args);
    }

    Expr* id(Name name) { return exprs_.at(name); }

    Bool *const_bool(Span span, type::Type *type, bool value) {
        return new (arena_.alloc<Bool>()) Bool(span, names_->fresh(), type, value);
    }

    I64* const_i64(Span span, type::Type* type, const char* digits_, std::size_t size) {
        char* digits = static_cast<char*>(arena_.alloc_array<char>(size));
        strncpy(digits, digits_, size);
        return new (arena_.alloc<I64>()) I64(span, names_->fresh(), type, digits);
    }

    Program build() { return Program(std::move(arena_), std::move(externs_)); }
};

} // namespace brmh::hossa

#endif // HOSSA_HPP
