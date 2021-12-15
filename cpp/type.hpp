#ifndef BRMH_TYPE_HPP
#define BRMH_TYPE_HPP

#include <ostream>

#include "name.hpp"

namespace brmh::type {

struct Types;

struct Type {
    virtual void print(Names const& names, std::ostream& dest) const = 0;
};

struct IntType : public Type {
    virtual void print(Names const& names, std::ostream& dest) const override;

private:
    friend struct Types;

    IntType();
};

struct Types {
    Types();

    IntType* get_int();

private:
    IntType* int_t_;
};

} // namespace brmh

#endif // BRMH_TYPE_HPP
