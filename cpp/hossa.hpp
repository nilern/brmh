#ifndef HOSSA_HPP
#define HOSSA_HPP

#include <span>
#include <ostream>

#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include "util.hpp"
#include "bumparena.hpp"
#include "name.hpp"
#include "type.hpp"

namespace brmh::hossa {

struct Fn;
struct Expr;
struct Param;
struct Builder;

// # Transfer

struct Transfer {
    virtual void print(Names& names, std::ostream& dest) const = 0;

    virtual void to_llvm(std::unordered_map<const hossa::Param*, llvm::Value*> env, llvm::LLVMContext& llvm_ctx, llvm::IRBuilder<>& builder) const = 0;

    Span span;

protected:
    Transfer(Span span);
};

// ## Return

struct Return : public Transfer {
    void print(Names& names, std::ostream& dest) const override;

    virtual void to_llvm(std::unordered_map<const hossa::Param*, llvm::Value*> env, llvm::LLVMContext& llvm_ctx, llvm::IRBuilder<>& builder) const override;

    Expr* res;

private:
    friend struct Builder;

    Return(Span span, Expr* res);
};

// # Block

struct Block {
    void print(Names& names, std::ostream& dest) const;

    Fn* fn;
    Name name;
    std::span<Param*> params;
    Transfer* transfer;

private:
    friend struct Builder;

    Block(Fn* fn, Name name, std::span<Param*> params, Transfer* transfer);
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

    virtual llvm::Value* to_llvm(std::unordered_map<const hossa::Param*, llvm::Value*>, llvm::LLVMContext& llvm_ctx, llvm::IRBuilder<>& builder) const = 0;

    opt_ptr<Block> block;
    Span span;
    Name name;
    type::Type* type;

protected:
    Expr(opt_ptr<Block> block, Span span, Name name, type::Type* type);
};

// ## Param

struct Param : public Expr {
    virtual void print(Names& names, std::ostream& dest) const override;

    virtual llvm::Value* to_llvm(std::unordered_map<const hossa::Param*, llvm::Value*>, llvm::LLVMContext& llvm_ctx, llvm::IRBuilder<>& builder) const override;

private:
    friend struct Builder;

    Param(opt_ptr<Block> block, Span span, Name name, type::Type* type);
};

// ## I64

struct I64 : public Expr {
    virtual void print(Names& names, std::ostream& dest) const override;

    virtual llvm::Value* to_llvm(std::unordered_map<const hossa::Param*, llvm::Value*>, llvm::LLVMContext& llvm_ctx, llvm::IRBuilder<>& builder) const override;

    const char* digits;

private:
    friend struct Builder;

    I64(opt_ptr<Block> block, Span span, Name name, type::Type* type, const char* digits);
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

    // OPTIMIZE: deduplicate constants:

    Fn* fn(Span span, Name name, type::Type* codomain, bool external, Block* entry);

    Block* block(Fn* fn, std::size_t arity, Transfer* transfer);
    Param* param(Span span, type::Type* type, Block* block, Name name, std::size_t index);
    Transfer* ret(Span span, Expr* res);

    Expr* id(Name name);
    I64* const_i64(Span span, type::Type* type, const char* chars, std::size_t size);

    Program build();

private:
    Names* names_;
    BumpArena arena_;
    std::unordered_map<Name, Expr*, Name::Hash> exprs_;
    std::vector<Fn*> externs_;
};

} // namespace brmh::hossa

#endif // HOSSA_HPP
