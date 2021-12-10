#ifndef BRMH_POS_H
#define BRMH_POS_H

#include <cstdint>

#include "filename.hpp"

namespace brmh {

struct Pos {
    Pos(const Filename filename, const uintptr_t index);
    explicit Pos(const Filename filename);
    Pos() = delete;

    Filename filename() const;
    uintptr_t index() const;

private:
    Filename filename_;
    uintptr_t index_;
};

} // namespace brmh

#endif // BRMH_POS_H
