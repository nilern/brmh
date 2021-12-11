#include "ast.hpp"

namespace brmh::ast {

Node::Node(Pos p) : pos(p) {}

Id::Id(Span span, const Name* n) : Node(span), name(n) {}

} // namespace brmh
