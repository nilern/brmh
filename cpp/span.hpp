#ifndef BRMH_SPAN_HPP
#define BRMH_SPAN_HPP

#include "pos.hpp"

namespace brmh {

struct Span {
    Pos start;
    Pos end;

    void print(std::ostream& dest) const {
        dest << '(';
        start.print(dest);
        dest << ") - (";
        end.print(dest);
        dest << ')';
    }
};

} // namespace brmh

#endif // BRMH_SPAN_HPP
