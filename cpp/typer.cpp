#include <cstring>

#include "type.hpp"
#include "ast.hpp"
#include "typeenv.hpp"
#include "fast.hpp"

namespace brmh {

fast::Program ast::Program::check(type::Types& types) {
    fast::Program program;
    TypeEnv env(types);

    for (auto def : defs) {
        def->declare(env);
    }

    for (auto def : defs) {
       program.push_toplevel(def->check(program, env));
    }

    return program;
}

void ast::FunDef::declare(TypeEnv &env) {
    env.declare(name, env.types().fn(domain(), codomain));
}

fast::Def* ast::FunDef::check(fast::Program& program, TypeEnv& parent_env) {
    TypeEnv env = parent_env.push_params();
    std::vector<fast::Param> new_params;
    for (const Param& param : params) {
        param.declare(env);
        new_params.push_back(program.param(param.span, param.name, param.type));
    }

    fast::Expr* typed_body = body->check(program, env, codomain);

    return program.fun_def(span, name, std::move(new_params), codomain, typed_body);
}

std::vector<type::Type*> ast::FunDef::domain() const {
    std::vector<type::Type*> res;
    for (const Param& param : params) {
        res.push_back(param.type);
    }
    return res;
}

void ast::Param::declare(TypeEnv &env) const {
    env.declare(name, type);
}

fast::Expr* ast::PrimApp::type_of(fast::Program& program, TypeEnv& env) const {
    switch (op) {
    case ast::PrimApp::Op::ADD_W_I64:
    case ast::PrimApp::Op::SUB_W_I64:
    case ast::PrimApp::Op::MUL_W_I64: {
        // FIXME: Brittle '2':s:
        if (args.size() != 2) { throw type::Error(span); }

        std::array<fast::Expr*, 2> typed_args;

        for (std::size_t i = 0; i < 2; ++i) {
            typed_args[i] = args[i]->check(program, env, env.types().get_i64());
        }

        switch (op) {
        case ast::PrimApp::Op::ADD_W_I64: return program.add_w_i64(span, env.types().get_i64(), typed_args);
        case ast::PrimApp::Op::SUB_W_I64: return program.sub_w_i64(span, env.types().get_i64(), typed_args);
        case ast::PrimApp::Op::MUL_W_I64: return program.mul_w_i64(span, env.types().get_i64(), typed_args);
        default: assert(false); // unreachable
        }
    }

    default: assert(false); // unreachable
    }
}

fast::Expr* ast::Int::type_of(fast::Program& program, TypeEnv& env) const {
    return program.const_i64(span, env.types().get_i64(), digits, strlen(digits));
}

fast::Expr* ast::Id::type_of(fast::Program& program, TypeEnv& env) const {
    std::optional<type::Type*> optType = env.find(name);
    if (optType) {
        return program.id(span, *optType, name);
    } else {
        throw type::Error(span);
    }
}

fast::Expr* ast::Expr::check(fast::Program& program, TypeEnv& env, type::Type* type) const {
    fast::Expr* expr = type_of(program, env);
    if (expr->type->is_subtype_of(type)) {
        return expr;
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

bool type::I64::is_subtype_of(const type::Type* other) const { return this == other; }

} // namespace brmh
