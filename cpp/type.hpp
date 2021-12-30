#ifndef BRMH_TYPE_HPP
#define BRMH_TYPE_HPP

#include <vector>
#include <ostream>

#include "llvm/IR/Type.h"

#include "span.hpp"
#include "name.hpp"
#include "error.hpp"

namespace brmh::type {

struct Types;

struct Type {
    virtual bool is_subtype_of(const Type* other) const = 0;

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    virtual llvm::Type* to_llvm(llvm::LLVMContext& llvm_ctx) const = 0;
};

struct FnType : public Type {
    virtual bool is_subtype_of(const Type* other) const override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual llvm::Type* to_llvm(llvm::LLVMContext& llvm_ctx) const override;

    std::vector<Type*> domain;
    Type* codomain;

private:
    friend struct Types;

    FnType(std::vector<Type*>&& domain, Type* codomain);
};

struct Bool : public Type {
    virtual bool is_subtype_of(const Type* other) const override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual llvm::Type* to_llvm(llvm::LLVMContext& llvm_ctx) const override;

private:
    friend struct Types;

    Bool();
};

struct I64 : public Type {
    virtual bool is_subtype_of(const Type* other) const override;

    virtual void print(Names const& names, std::ostream& dest) const override;

    virtual llvm::Type* to_llvm(llvm::LLVMContext& llvm_ctx) const override;

private:
    friend struct Types;

    I64();
};

struct Types {
    Types();

    Bool* get_bool();
    I64* get_i64();
    FnType* fn(std::vector<Type*>&& domain, Type* codomain);

private:
    Bool* bool_;
    I64* i64_;
};

class Error : public BrmhError {
public:
    explicit Error(Span span);

    virtual const char* what() const noexcept override;

    Span span;
};

} // namespace brmh

#endif // BRMH_TYPE_HPP
