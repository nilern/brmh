#include "error.hpp"

namespace brmh {

const char* BrmhError::what() const noexcept { return "BrmhError"; }

} // namespace brmh
