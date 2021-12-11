#include "ast.hpp"

namespace brmh::ast {

Node::Node(Span sp) : span(sp) {}

Id::Id(Span span, const Name* n) : Node(span), name(n) {}

} // namespace brmh
