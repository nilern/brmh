#ifndef BRMH_POS_H
#define BRMH_POS_H

#include <ostream>

#include "filename.hpp"

namespace brmh {

struct Pos {
    Filename filename;
    std::size_t line;
    std::size_t column;

    Pos(Filename filename_, std::size_t line_, std::size_t column_)
        : filename(filename_), line(line_), column(column_) {}

    void print(std::ostream& out) const;

    Pos next(char c) {
        return c != '\n' ?
                    Pos(filename, line, column + 1)
                  : Pos(filename, line + 1, 1);
    }
};

} // namespace brmh

#endif // BRMH_POS_H
