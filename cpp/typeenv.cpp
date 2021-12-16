#include "typeenv.hpp"

namespace brmh {

TypeEnv::TypeEnv(type::Types& types, TypeEnv* parent) : types_(types), bindings_(), parent_(parent) {}

TypeEnv::TypeEnv(type::Types& types) : types_(types), bindings_(), parent_() {}

type::Types& TypeEnv::types() const { return types_; }

std::optional<type::Type*> TypeEnv::find(Name name) const {
    auto it = bindings_.find(name);
    return it != bindings_.end()
            ? it->second
            : parent_
              ? parent_.value()->find(name)
              : std::optional<type::Type*>();
}

TypeEnv TypeEnv::push_params() {
    return TypeEnv(types_, this);
}

void TypeEnv::declare(Name name, type::Type* type) {
    bindings_.insert({name, type});
}

} // namespace brmh
