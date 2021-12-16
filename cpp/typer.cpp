#include <cstring>

#include "type.hpp"
#include "ast.hpp"
#include "typeenv.hpp"

namespace brmh {

ast::Program ast::Program::check(type::Types& types) {
    TypeEnv env(types);

    for (auto def : defs) {
        def->declare(env);
    }

    std::vector<Def*> typed_defs;
    for (auto def : defs) {
       typed_defs.push_back(def->check(env));
    }

    return ast::Program(std::move(typed_defs));
}

void ast::FunDef::declare(TypeEnv &env) {
    env.declare(name, env.types().fn(domain(), codomain));
}

ast::Def* ast::FunDef::check(TypeEnv& parent_env) {
    TypeEnv env = parent_env.push_params();
    for (auto param : params) {
        param.declare(env);
    }

    ast::Expr* typed_body = body->check(env, codomain);

    return new FunDef(span, name, std::vector(params), codomain, typed_body);
}

std::vector<type::Type*> ast::FunDef::domain() const {
    std::vector<type::Type*> res;
    for (const auto param : params) {
        res.push_back(param.type);
    }
    return res;
}

void ast::Param::declare(TypeEnv &env) const {
    env.declare(name, type);
}

std::pair<ast::Expr*, type::Type*> ast::Int::type_of(TypeEnv& env) const {
    return {new Int(span, digits, strlen(digits)), env.types().get_int()};
}

std::pair<ast::Expr*, type::Type*> ast::Id::type_of(TypeEnv& env) const {
    std::optional<type::Type*> optType = env.find(name);
    if (optType) {
        return {new Id(span, name), *optType};
    } else {
        throw type::Error(span);
    }
}

ast::Expr* ast::Expr::check(TypeEnv& env, type::Type* type) const {
    std::pair<ast::Expr*, type::Type*> typing = type_of(env);
    if (typing.second->is_subtype_of(type)) {
        return typing.first;
    } else {
        throw type::Error(span);
    }
}

// # Subtyping

bool type::FnType::is_subtype_of(const type::Type* other) const {
    const type::FnType* other_fn = dynamic_cast<const type::FnType*>(other); // OPTIMIZE
    if (!other_fn) { return false; }

    auto dom = domain.begin();
    auto other_dom = other_fn->domain.begin();
    for (;; ++dom, ++other_dom) {
        if (dom != domain.end()) {
            if (other_dom != other_fn->domain.end()) {
                if (!(*other_dom)->is_subtype_of(*dom)) {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            if (other_dom == other_fn->domain.end()) {
                break;
            } else {
                return false;
            }
        }
    }

    return codomain->is_subtype_of(other_fn->codomain);
}

bool type::IntType::is_subtype_of(const type::Type* other) const { return this == other; }

} // namespace brmh
