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

struct Fn;
struct Block;
struct Expr;
struct Param;
struct Builder;

// # Transfer

struct Transfer {
    virtual void print(Names& names, std::ostream& dest, std::unordered_set<Block const*>& visited) const = 0;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const = 0;

    Span span;

protected:
    Transfer(Span span);
};

// ## If

struct If : public Transfer {
    void print(Names& names, std::ostream& dest, std::unordered_set<Block const*>& visited) const override;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

    Expr* cond;
    Block* conseq;
    Block* alt;

private:
    friend struct Builder;

    If(Span span, Expr* cond, Block* conseq, Block* alt);
};

// ## Goto

struct Goto : public Transfer {
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
    void print(Names& names, std::ostream& dest, std::unordered_set<Block const*>& visited) const override;

    virtual void to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const override;

    Expr* res;

private:
    friend struct Builder;

    Return(Span span, Expr* res);
};

// # Block

struct Block {
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

struct Fn {
    void print(Names& names, std::ostream& dest) const;

    void to_llvm(Names const& names, llvm::LLVMContext& llvm_ctx, llvm::Module& module, llvm::Function::LinkageTypes linkage) const;

    Span span;
    Name name;
    type::Type* codomain;
    Block* entry;

private:
    friend struct Builder;

    Fn(Span span, Name name, type::Type* codomain, Block* entry);
};

// # Expr

struct Expr {
    virtual void print(Names& names, std::ostream& dest) const = 0;

    virtual llvm::Value* to_llvm(ToLLVMCtx& ctx, llvm::IRBuilder<>& builder) const = 0;

    Span span;
    Name name;
    type::Type* type;

protected:
    Expr(Span span, Name name, type::Type* type);
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

// # Builder

struct Builder {
    Builder(Names* names);

    Names* names() const;

    // OPTIMIZE: deduplicate constants:

    Fn* fn(Span span, Name name, type::Type* codomain, bool external, Block* entry);

    opt_ptr<Block> current_block() const;
    void set_current_block(Block* block);

    Block* block(std::size_t arity, Transfer* transfer);
    Param* param(Span span, type::Type* type, Block* block, Name name, std::size_t index);

    Transfer* if_(Span span, Expr* cond, Block* conseq, Block* alt);
    Transfer* goto_(Span span, Block* dest, Expr* res);
    Transfer* ret(Span span, Expr* res);

    Expr* add_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);
    Expr* sub_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);
    Expr* mul_w_i64(Span span, type::Type* type, std::array<Expr*, 2> args);
    Expr* id(Name name);
    Bool* const_bool(Span span, type::Type* type, bool value);
    I64* const_i64(Span span, type::Type* type, const char* chars, std::size_t size);

    Program build();

private:
    Names* names_;
    BumpArena arena_;
    std::unordered_map<Name, Expr*, Name::Hash> exprs_;
    std::vector<Fn*> externs_;
    opt_ptr<Block> current_block_;
};

} // namespace brmh::hossa

#endif // HOSSA_HPP
