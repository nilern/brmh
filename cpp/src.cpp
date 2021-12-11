#include "src.hpp"

#include <string>
#include <iostream>
#include <sstream>

namespace brmh {

Src::Src(Filename filename, std::string&& source_code) : filename_(filename), source_code_(source_code) {}

Src Src::cli_arg(const char* source_code) { return Src(Filename("<CLI arg>"), std::string(source_code)); }

Src Src::stdin() {
    std::stringstream ss;
    ss << std::cin.rdbuf();
    return Src(Filename("<stdin>"), ss.str());
}

Filename Src::filename() const { return filename_; }

const char* Src::source_code() const { return source_code_.c_str(); }

} // namespace brmh
