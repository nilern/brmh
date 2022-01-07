#include "type.hpp"

#include "llvm/IR/DerivedTypes.h"

namespace brmh::type {

// # Type

// ## FnType

FnType::FnType(std::vector<Type*>&& domain_, Type* codomain_) : domain(domain_), codomain(codomain_) {}

void FnType::print(Names const& names, std::ostream& dest) const {
    dest << "fn (";

    auto it = domain.begin();
    if (it != domain.end()) {
        (*it)->print(names, dest);

        for (; it != domain.end(); ++it) {
            dest << ", ";
            (*it)->print(names, dest);
        }
    }

    dest << ") -> ";

    codomain->print(names, dest);
}

llvm::Type *FnType::to_llvm(llvm::LLVMContext &llvm_ctx) const {
    std::vector<llvm::Type*> llvm_domain(domain.size());
    std::transform(domain.begin(), domain.end(), llvm_domain.begin(), [&] (Type* dom) {
        return dom->to_llvm(llvm_ctx);
    });
    return llvm::FunctionType::get(codomain->to_llvm(llvm_ctx), llvm_domain, false);
}

// ## Bool

Bool::Bool() {}

void Bool::print(Names const&, std::ostream& dest) const { dest << "bool"; }

llvm::Type* Bool::to_llvm(llvm::LLVMContext& llvm_ctx) const {
    return llvm::Type::getInt8Ty(llvm_ctx);
}

// ## I64

I64::I64() {}

void I64::print(Names const&, std::ostream& dest) const { dest << "i64"; }

llvm::Type *I64::to_llvm(llvm::LLVMContext& llvm_ctx) const {
    return llvm::Type::getInt64Ty(llvm_ctx);
}

// # Types

Types::Types() : bool_(new Bool()), i64_(new I64()) {}

Bool* Types::get_bool() { return bool_; }

I64* Types::get_i64() { return i64_; }

FnType* Types::fn(std::vector<Type*>&& domain, Type* codomain) {
    return new FnType(std::move(domain), codomain);  // OPTIMIZE
}

// # Error

Error::Error(Span span_) : BrmhError(), span(span_) {}

const char* Error::what() const noexcept { return "TypeError"; }

} // namespace brmh
