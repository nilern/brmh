#include "pos.hpp"

#include "filename.hpp"

namespace brmh {

void Pos::print(std::ostream& out) const {
    out << filename.c_str() << " @ " << line << ':' << column;
}

} // namespace brmh
