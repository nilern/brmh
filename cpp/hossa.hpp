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

// FIXME: Printing should display sharing in the dataflow DAG instead of treating it as a tree

namespace brmh {

struct ToLLVMCtx;

}

namespace brmh::hossa {

struct Expr;
struct Transfer;
struct Block;
struct Fn;
struct Builder;

// # Visitors

class TransfersExprsVisitor {
    virtual void visit(Transfer const* transfer) = 0;
    virtual void visit(Expr const* transfer) = 0;
};

// # Expr

struct Expr {
    virtual std::span<Expr*> args() const = 0;

    virtual void print(Names& names, std::ostream& dest) const = 0;

    virtual llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const = 0;

    Span span;
    Name name;
    type::Type* type;

protected:
    Expr(Span span, Name name, type::Type* type);
};

// ## Call

struct Call : public Expr {
    Expr* callee;
    std::span<Expr*> args;

    virtual void print(Names& names, std::ostream& dest) const override {
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

    virtual llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

private:
    friend struct Builder;

    Call(Span span, Name name, type::Type* type, Expr* callee_, std::span<Expr*> args_)
        : Expr(span, name, type), callee(callee_), args(args_) {}
};

// ## PrimApp

template<std::size_t N>
struct PrimApp : public Expr {
    virtual void print(Names& names, std::ostream& dest) const override {
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

    virtual const char* opname() const = 0;

    std::array<Expr*, N> args;

protected:
    PrimApp(Span span, Name name, type::Type* type, std::array<Expr*, N> args_)
        : Expr(span, name, type), args(args_) {}
};

struct AddWI64 : public PrimApp<2> {
    virtual const char* opname() const override;

    virtual llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

private:
    friend struct Builder;

    AddWI64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args);
};

struct SubWI64 : public PrimApp<2> {
    virtual const char* opname() const override;

    virtual llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

private:
    friend struct Builder;

    SubWI64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args);
};

struct MulWI64 : public PrimApp<2> {
    virtual const char* opname() const override;

    virtual llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

private:
    friend struct Builder;

    MulWI64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args);
};

struct EqI64 : public PrimApp<2> {
    virtual const char* opname() const override { return "eqI64"; }

    virtual llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

private:
    friend struct Builder;

    EqI64(Span span, Name name, type::Type* type, std::array<Expr*, 2> args) : PrimApp(span, name, type, args) {}
};

// ## Param

struct Param : public Expr {
    virtual void print(Names& names, std::ostream& dest) const override;

    virtual llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

private:
    friend struct Builder;

    Param(Span span, Name name, type::Type* type);
};

// ## I64

struct I64 : public Expr {
    virtual void print(Names& names, std::ostream& dest) const override;

    virtual llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

    const char* digits;

private:
    friend struct Builder;

    I64(Span span, Name name, type::Type* type, const char* digits);
};

// ## Bool

struct Bool : public Expr {
    virtual void print(Names&, std::ostream& dest) const override {
        dest << (value ? "True" : "False");
    }

    virtual llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

    bool value;

private:
    friend struct Builder;

    Bool(Span span, Name name, type::Type* type, bool v) : Expr(span, name, type), value(v) {}
};

// # Transfer

struct Transfer {
    virtual std::span<Expr*> args() const = 0;
    virtual std::span<Block*> successors() = 0;

    virtual void print(Names& names, std::ostream& dest, std::unordered_set<Block const*>& visited) const = 0;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const = 0;

    Span span;

protected:
    Transfer(Span span);
};

// ## If

struct If : public Transfer {
    virtual std::span<Block*> successors() override {
        return std::span<Block*>(&conseq, 2);
    }

    void print(Names& names, std::ostream& dest, std::unordered_set<Block const*>& visited) const override;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

    Expr* cond;
    Block* conseq;
    Block* alt;

private:
    friend struct Builder;

    If(Span span, Expr* cond, Block* conseq, Block* alt);
};

// ## TailCall

struct TailCall : public Transfer {
    Expr* callee;
    std::span<Expr*> args;

    virtual std::span<Block*> successors() override {
        return std::span<Block*>();
    }

    void print(Names& names, std::ostream& dest, std::unordered_set<Block const*>&) const override {
        dest << "        tailcall ";
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

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

private:
    friend struct Builder;

    TailCall(Span span, Expr* callee_, std::span<Expr*> args_)
        : Transfer(span), callee(callee_), args(args_) {}
};

// ## Goto

struct Goto : public Transfer {
    virtual std::span<Block*> successors() override {
        return std::span<Block*>(&dest, 1);
    }

    void print(Names& names, std::ostream& dest, std::unordered_set<Block const*>& visited) const override;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

    Block* dest;
    Expr* res;

private:
    friend struct Builder;

    Goto(Span span, Block* dest, Expr* res);
};

// ## Return

struct Return : public Transfer {
    virtual std::span<Block*> successors() override {
        return std::span<Block*>();
    }

    void print(Names& names, std::ostream& dest, std::unordered_set<Block const*>& visited) const override;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

    Expr* res;

private:
    friend struct Builder;

    Return(Span span, Expr* res);
};

// # Program

struct Program {
    void print(Names& names, std::ostream& dest) const;

    void to_llvm(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module) const;

    std::vector<Fn*> externs;

private:
    friend struct Builder;

    Program(BumpArena&& arena, std::vector<Fn*>&& externs);

    BumpArena arena_;
};

// # Block

struct Block {
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

    template<typename F>
    void post_visit(F f) const {
        std::unordered_set<Block const*> visited;
        do_post_visit(visited, f);
    }

    void print(Names& names, std::ostream& dest, std::unordered_set<Block const*>& visited) const;

    llvm::BasicBlock* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const;

    Name name;
    std::span<Param*> params;
    Transfer* transfer;

private:
    friend struct Builder;

    Block(Name name, std::span<Param*> params, Transfer* transfer);
};

// # Fn

struct Fn : public Expr {
    Block* entry;

    virtual void post_visit_transfers_and_exprs(TransfersExprsVisitor& visitor) const;

    virtual llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

    void print(Names& names, std::ostream& dest) const override;
    void print_def(Names& names, std::ostream& dest) const;

    void llvm_declare(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module, llvm::Function::LinkageTypes linkage) const;
    void llvm_define(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module) const;

private:
    friend struct Builder;

    Fn(Span span, Name name, type::FnType* type, Block* entry);
};

// # Builder

struct Builder {
    Builder(Names* names, type::Types& types);

    Names* names() const;
    type::Types& types() const { return types_; }

    // OPTIMIZE: deduplicate constants:

    Fn* fn(Span span, Name name, type::FnType* type, bool external, Block* entry);
    Fn* get_fn(Name name) const { return static_cast<Fn*>(exprs_.at(name)); }

    opt_ptr<Block> current_block() const;
    void set_current_block(Block* block);

    Block* block(std::size_t arity, Transfer* transfer);
    Param* param(Span span, type::Type* type, Block* block, Name name, std::size_t index);

    std::span<Expr*> args(std::size_t arity) {
        return std::span<Expr*>(static_cast<Expr**>(arena_.alloc_array<Expr*>(arity)), arity);
    }

    Transfer* if_(Span span, Expr* cond, Block* conseq, Block* alt);
    Transfer* tail_call(Span span, Expr* callee, std::span<Expr*> args) {
        return new (arena_.alloc<TailCall>()) TailCall(span, callee, args);
    }
    Transfer* goto_(Span span, Block* dest, Expr* res);
    Transfer* ret(Span span, Expr* res);

    Expr* call(Span span, type::Type* type, Expr* callee, std::span<Expr*> args) {
        return new (arena_.alloc<Call>()) Call(span, names_->fresh(), type, callee, args);
    }
    Expr* add_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);
    Expr* sub_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);
    Expr* mul_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);

    Expr* eq_i64(Span span, type::Type* type, std::array<Expr*, 2> args) {
        return new (arena_.alloc<EqI64>()) EqI64(span, names_->fresh(), type, args);
    }

    Expr* id(Name name);
    Bool* const_bool(Span span, type::Type* type, bool value);
    I64* const_i64(Span span, type::Type* type, const char* chars, std::size_t size);

    Program build();

private:
    Names* names_;
    type::Types& types_;
    BumpArena arena_;
    std::unordered_map<Name, Expr*, Name::Hash> exprs_;
    std::vector<Fn*> externs_;
    opt_ptr<Block> current_block_;
};

} // namespace brmh::hossa

#endif // HOSSA_HPP
