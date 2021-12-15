#include "type.hpp"

namespace brmh::type {

IntType::IntType() {}

void IntType::print(Names const&, std::ostream& dest) const { dest << "int"; }

// # Types

Types::Types() : int_t_(new IntType()) {}

IntType* Types::get_int() { return int_t_; }

} // namespace brmh
