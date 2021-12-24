#include "src.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

namespace brmh {

Src::Src(Filename filename, std::string&& source_code) : filename_(filename), source_code_(source_code) {}

Src Src::file(char const* filename) {
    // FIXME: Error handling
    std::fstream infile(filename, std::ios::in);
    std::stringstream ss;
    ss << infile.rdbuf();
    return Src(Filename("<stdin>"), ss.str());
}

Filename Src::filename() const { return filename_; }

const char* Src::source_code() const { return source_code_.c_str(); }

} // namespace brmh
