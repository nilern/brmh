#ifndef HOSSA_HPP
#define HOSSA_HPP

#include <span>
#include <unordered_set>
#include <ostream>

#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include "../util.hpp"
#include "../bumparena.hpp"
#include "../name.hpp"
#include "../type.hpp"

namespace brmh {

struct ToLLVMCtx;

}

namespace brmh::cps {

struct Expr;
struct Transfer;
struct Cont;
struct Block;
struct Return;
struct Fn;
class Builder;

// # Util Classes

struct PrintCtx {
    Names const& names;
    std::unordered_map<Block const*, std::vector<Expr const*>> block_exprs;
    std::unordered_set<Expr const*> visited_exprs;

    PrintCtx(Names const& names_,
             std::unordered_map<Block const*, std::vector<Expr const*>>&& block_exprs_,
             std::unordered_set<Expr const*>&& visited_exprs_)
        : names(names_), block_exprs(block_exprs_), visited_exprs(visited_exprs_) {}
};

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

    virtual void do_print(Names const& names, std::ostream& dest) const = 0;

    void print_in(PrintCtx& ctx, std::ostream& dest) const;

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const = 0;
    llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const;
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

    virtual void do_print(Names const& names, std::ostream& dest) const override {
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

    void do_print(Names const& names, std::ostream& dest) const override {
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
    void do_print(Names const&, std::ostream& dest) const override {
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
    virtual void do_print(Names const&, std::ostream& dest) const override {
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
    virtual std::span<Cont* const> successors() const = 0;

    virtual void do_print(Names const& names, std::ostream& dest) const = 0;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder, Block const* block) const = 0;
};

// ## If

struct If : public Transfer {
    Expr* cond;
    Cont* conseq;
    Cont* alt;

private:
    friend class Builder;

    If(Span span, Expr *cond_, Cont* conseq_, Cont* alt_)
        : Transfer(span), cond(cond_), conseq(conseq_), alt(alt_) {}

public:
    virtual std::span<Expr* const> operands() const override {
        return std::span<Expr* const>(&cond, 1);
    }

    virtual std::span<Cont* const> successors() const override {
        return std::span<Cont* const>(&conseq, 2);
    }

    void do_print(Names const& names, std::ostream& dest) const override;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder, Block const* block) const override;
};

// ## Call

struct Call : public Transfer {
    std::span<Expr*> exprs;
    Cont* cont;

private:
    friend class Builder;

    Call(Span span, std::span<Expr*> exprs_, Cont* cont_)
        : Transfer(span), exprs(exprs_), cont(cont_) {}

public:
    Expr* callee() const { return exprs[0]; }

    std::span<Expr* const> args() const { return exprs.subspan(1); }

    virtual std::span<Expr* const> operands() const override { return exprs; }

    virtual std::span<Cont* const> successors() const override {
        return std::span<Cont* const>(&cont, 1);
    }

    void do_print(Names const& names, std::ostream& dest) const override;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder, Block const* block) const override;
};

// ## Goto

struct Goto : public Transfer {
    Cont* dest;
    Expr* res;

private:
    friend class Builder;

    Goto(Span span, Cont* dest_, Expr* res_) : Transfer(span), dest(dest_), res(res_) {}

public:
    virtual std::span<Expr* const> operands() const override {
        return std::span<Expr* const>(&res, 1);
    }

    virtual std::span<Cont* const> successors() const override {
        return std::span<Cont* const>(&dest, 1);
    }

    void do_print(Names const& names, std::ostream& desto) const override;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder, Block const* block) const override;
};

// # Cont

// ## Visitor

struct ContVisitor {
    virtual void visit(Block const* block) = 0;
    virtual void visit(Return const* ret) = 0;
};

// ## Cont

struct Cont {
    Name name;

protected:
    Cont(Name name_) : name(name_) {}

public:
    virtual void accept(ContVisitor& visitor) = 0;

    virtual opt_ptr<Block const> as_block() const = 0;

    virtual void print(PrintCtx& ctx, std::ostream& dest) const = 0;
};

// ## Return

struct Return : public Cont {
private:
    friend class Builder;

    Return(Name name) : Cont(name) {}

public:
    virtual void accept(ContVisitor& visitor) override { visitor.visit(this); }

    virtual opt_ptr<Block const> as_block() const override { return opt_ptr<Block const>::none(); }

    virtual void print(PrintCtx& ctx, std::ostream& dest) const override {
        name.print(ctx.names, dest);
    }
};

// ## Block

struct Block : public Cont {
    std::span<Param*> params;
    Transfer* transfer;

private:
    friend class Builder;

    Block(Name name, std::span<Param*> params_, Transfer* transfer_)
        : Cont(name), params(params_), transfer(transfer_) {}

public:
    virtual void accept(ContVisitor& visitor) override { visitor.visit(this); }

    virtual opt_ptr<Block const> as_block() const override { return opt_ptr<Block const>::some(this); }

    virtual void print(PrintCtx& ctx, std::ostream& dest) const override;

    void llvm_declare(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const;
    void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const;
    void llvm_patch_phis(ToLLVMCtx& ctx) const;

    template<typename F>
    void do_post_visit(std::unordered_set<Block const*>& visited, F f) const {
        if (!visited.contains(this)) {
            visited.insert(this);

            for (Cont const* succ : transfer->successors()) {
                succ->as_block().iter([&] (Block const* succ) {
                    return succ->do_post_visit(visited, f);
                });
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
};

// # Fn

struct Fn : public Expr {
    Return* ret;
    Block* entry;

private:
    friend class Builder;

    Fn(Span span, Name name, type::FnType* type, Return* ret_, Block* entry_)
        : Expr(span, name, type), ret(ret_), entry(entry_) {}

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

    void do_print(Names const& names, std::ostream &dest) const override {
        dest << "fun ";
        name.print(names, dest);
    }

    void print_def(Names const& names, std::ostream& dest) const;

    virtual llvm::Value* do_to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;
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
    void print(Names const& names, std::ostream& dest) const {
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

    void define(Name name, Expr* expr) { exprs_.insert({name, expr}); }

    // OPTIMIZE: deduplicate constants, constant folding, CSE etc.:

    Fn* fn(Span span, Name name, type::FnType* type, bool external, Return* ret, Block* entry) {
        Fn* const res = new (arena_.alloc<Fn>()) Fn(span, name, type, ret, entry);
        define(name, res);
        if (external) { externs_.push_back(res); }
        return res;
    }

    Fn* get_fn(Name name) const { return static_cast<Fn*>(exprs_.at(name)); }

    Block* block(std::size_t arity, Transfer* transfer) {
        std::span<Param*> const params = std::span(static_cast<Param**>(arena_.alloc_array<Param*>(arity)), arity);
        return new (arena_.alloc<Block>()) Block(names_->fresh(), params, transfer);
    }

    Return* return_(Name name) { return new (arena_.alloc<Return>()) Return(name); }

    Param* param(Span span, type::Type* type, Block* block_, Name name, std::size_t index) {
        Param* const param = new (arena_.alloc<Param>()) Param(span, name, type);

        block_->params[index] = param;

        define(name, param);

        return param;
    }

    std::span<Expr*> args(std::size_t arity) {
        return std::span<Expr*>(static_cast<Expr**>(arena_.alloc_array<Expr*>(arity)), arity);
    }

    Transfer *if_(Span span, Expr *cond, Block *conseq, Block *alt) {
        return new (arena_.alloc<If>()) If(span, cond, conseq, alt);
    }

    Transfer* call(Span span, std::span<Expr*> exprs, Cont* cont) {
        return new (arena_.alloc<Call>()) Call(span, exprs, cont);
    }

    Transfer *goto_(Span span, Cont* dest, Expr *res) {
        return new (arena_.alloc<Goto>()) Goto(span, dest, res);
    }

    Expr* add_w_i64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args) {
        return new (arena_.alloc<AddWI64>()) AddWI64(span, name, type, args);
    }

    Expr* sub_w_i64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args) {
        return new (arena_.alloc<SubWI64>()) SubWI64(span, name, type, args);
    }

    Expr* mul_w_i64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args) {
        return new (arena_.alloc<MulWI64>()) MulWI64(span, name, type, args);
    }

    Expr* eq_i64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args) {
        return new (arena_.alloc<EqI64>()) EqI64(span, name, type, args);
    }

    Expr* id(Name name) { return exprs_.at(name); }

    Bool *const_bool(Span span, Name name, type::Type *type, bool value) {
        return new (arena_.alloc<Bool>()) Bool(span, name, type, value);
    }

    I64* const_i64(Span span, Name name, type::Type* type, const char* digits_, std::size_t size) {
        char* digits = static_cast<char*>(arena_.alloc_array<char>(size));
        strncpy(digits, digits_, size);
        return new (arena_.alloc<I64>()) I64(span, name, type, digits);
    }

    Program build() { return Program(std::move(arena_), std::move(externs_)); }
};

} // namespace brmh::cps

#endif // HOSSA_HPP
