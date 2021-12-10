#include "filename.hpp"

#include <cstring>

namespace brmh {

Filename::Filename(const char* chars) : chars_(strdup(chars)) {}

Filename::~Filename() {
/* There arent that many filenames and we will need them for the whole compile
 * so don't bother freeing the string. This also keeps `Filename` trivially copyable. */
}

Filename Filename::create(const char* chars) { return Filename(chars); }

} // namespace brmh
