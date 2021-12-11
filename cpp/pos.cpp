#include "pos.hpp"

#include "filename.hpp"

namespace brmh {

Pos::Pos(const Filename filename, const uintptr_t index) : filename_(filename), index_(index) {}

Filename Pos::filename() const { return filename_; }

uintptr_t Pos::index() const { return index_; }

void Pos::print(std::ostream& out) const {
    out << filename_.c_str() << " @ " << index_;
}

} // namespace brmh
