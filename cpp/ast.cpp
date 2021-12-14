#include "ast.hpp"

namespace brmh::ast {

// # Node

Node::Node(Span sp) : span(sp) {}

Node::~Node() {}

// # Id

Id::Id(Span span, Name n) : Node(span), name(n) {}

Id::~Id() {}

void Id::print(Names const& names, std::ostream& dest) const { name.print(names, dest); }

} // namespace brmh
