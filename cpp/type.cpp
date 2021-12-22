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

    dest << " -> ";

    codomain->print(names, dest);
}

llvm::Type *FnType::to_llvm(llvm::LLVMContext &llvm_ctx) const {
    std::vector<llvm::Type*> llvm_domain(domain.size());
    std::transform(domain.begin(), domain.end(), llvm_domain.begin(), [&] (Type* dom) {
        return dom->to_llvm(llvm_ctx);
    });
    return llvm::FunctionType::get(codomain->to_llvm(llvm_ctx), llvm_domain, false);
}

// ## IntType

IntType::IntType() {}

void IntType::print(Names const&, std::ostream& dest) const { dest << "int"; }

llvm::Type *IntType::to_llvm(llvm::LLVMContext& llvm_ctx) const {
    return llvm::Type::getInt64Ty(llvm_ctx);
}

// # Types

Types::Types() : int_t_(new IntType()) {}

IntType* Types::get_int() { return int_t_; }

FnType* Types::fn(std::vector<Type*>&& domain, Type* codomain) {
    return new FnType(std::move(domain), codomain);  // OPTIMIZE
}

// # Error

Error::Error(Span span_) : BrmhError(), span(span_) {}

const char* Error::what() const noexcept { return "TypeError"; }

} // namespace brmh
