#ifndef BRMH_TYPEENV_HPP
#define BRMH_TYPEENV_HPP

#include <optional>

#include "type.hpp"

namespace brmh {

struct TypeEnv {
    explicit TypeEnv(type::Types& types);

    type::Types& types() const;

    std::optional<type::Type*> find(Name name) const;

    TypeEnv push_params();
    void declare(Name name, type::Type* type);

private:
    TypeEnv(type::Types& types, TypeEnv* parent);

    type::Types& types_;
    std::unordered_map<Name, type::Type*, Name::Hash> bindings_;
    std::optional<TypeEnv*> parent_;
};

} // namespace brmh

#endif // BRMH_TYPEENV_HPP
