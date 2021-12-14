#ifndef BRMH_AST_HPP
#define BRMH_AST_HPP

#include "span.hpp"
#include "name.hpp"

namespace brmh::ast {

struct Node {
    explicit Node(Span pos);
    virtual ~Node();

    virtual void print(Names const& names, std::ostream& dest) const = 0;

    Span span;
};

struct Id : Node {
    Id(Span pos, Name name);
    ~Id();

    virtual void print(Names const& names, std::ostream& dest) const override;

    Name name;
};

} // namespace brmh::ast

#endif // BRMH_AST_HPP
