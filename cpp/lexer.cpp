#include "lexer.hpp"

namespace brmh {

Lexer::Lexer(const char* chars) : chars(chars) {}

optional<char> Lexer::peek() {
    return *chars ? optional(*chars) : optional<char>();
}

optional<char> Lexer::next() {
    const optional<char> c = peek();
    ++chars;
    return c;
}

} // namespace brmh
