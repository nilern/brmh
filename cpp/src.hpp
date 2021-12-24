#ifndef BRMH_SRC_HPP
#define BRMH_SRC_HPP

#include <string>

#include "filename.hpp"

namespace brmh {

struct Src {
    Src() = delete;

    static Src file(const char* filename);

    Filename filename() const;
    const char* source_code() const;

private:
    Src(Filename filename, std::string&& source_code);

    Filename filename_;
    std::string source_code_;
};

} // namespace brmh

#endif // BRMH_SRC_HPP
