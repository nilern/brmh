#include "ast.hpp"

namespace brmh::ast {

// # Node

Node::Node(Span sp) : span(sp) {}

Node::~Node() {}

// # Id

Id::Id(Span span, const Name* n) : Node(span), name(n) {}

Id::~Id() {}

void Id::print(std::ostream& dest) const { name->print(dest); }

} // namespace brmh
