#include "type.hpp"

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

// ## IntType

IntType::IntType() {}

void IntType::print(Names const&, std::ostream& dest) const { dest << "int"; }

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
