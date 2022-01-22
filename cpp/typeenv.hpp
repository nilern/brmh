#ifndef BRMH_TYPEENV_HPP
#define BRMH_TYPEENV_HPP

#include <optional>

#include "type.hpp"
#include "fast.hpp"

namespace brmh {

class TypeEnv {
    Names& names_;
    type::Types& types_;
    std::unordered_map<Name, std::pair<Name, type::Type*>, Name::Hash> bindings_;
    std::optional<TypeEnv*> parent_;

    TypeEnv(Names& names, type::Types& types, TypeEnv* parent)
        : names_(names), types_(types), bindings_(), parent_(parent) {}

public:
    TypeEnv(Names& names, type::Types& types)
        : names_(names), types_(types), bindings_(), parent_() {}

    type::Types& types() const { return types_; }

    std::optional<std::pair<Name, type::Type*>> find(Name name) const {
        auto it = bindings_.find(name);
        return it != bindings_.end()
                ? it->second
                : parent_
                  ? parent_.value()->find(name)
                  : std::optional<std::pair<Name, type::Type*>>();
    }

    TypeEnv push_frame() { return TypeEnv(names_, types_, this); }

    Name declare(Name name, type::Type* type) {
        Name const unique_name = names_.freshen(name);
        bindings_.insert({name, {unique_name, type}});
        return unique_name;
    }

    type::Uv* uv() const { return types_.uv(); }
};

} // namespace brmh

#endif // BRMH_TYPEENV_HPP
