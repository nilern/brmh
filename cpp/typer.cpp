#include <cstring>

#include "type.hpp"
#include "ast.hpp"
#include "typeenv.hpp"
#include "fast.hpp"

namespace brmh {

// # Program

fast::Program ast::Program::check(Names& names, type::Types& types) {
    fast::Program program;
    TypeEnv env(names, types);

    for (auto def : defs) {
        def->declare(env);
    }

    for (auto def : defs) {
       program.push_toplevel(def->check(program, env));
    }

    return program;
}

// # Defs

void ast::FunDef::declare(TypeEnv &env) {
    env.declare(name, env.uv());
}

fast::Def* ast::FunDef::check(fast::Program& program, TypeEnv& parent_env) {
    auto const binding = parent_env.find(name).value();
    Name const unique_name = binding.first;

    TypeEnv env = parent_env.push_frame();
    std::vector<fast::Pat*> new_params;
    std::vector<type::Type*> domain;
    for (Pat const* param : params) {
        fast::Pat* const new_param = param->type_of(program, env);
        new_params.push_back(new_param);
        domain.push_back(new_param->type);
    }

    fast::Expr* typed_body = body->check(program, env, codomain);

    binding.second->unify(env.types().fn(std::move(domain), codomain), span);

    return program.fun_def(span, unique_name, std::move(new_params), codomain, typed_body);
}

// # Expressions

fast::Expr* type_block(fast::Program& program, TypeEnv& env,
                       std::vector<ast::Stmt*> const& stmts, std::size_t i,
                       ast::Expr const* body,
                       std::span<fast::Stmt*> typed_stmts)
{
    if (i < stmts.size()) {
        auto stmt_env = stmts[i]->check(program, env);
        typed_stmts[i] = stmt_env.first;
        return type_block(program, stmt_env.second, stmts, i + 1, body, typed_stmts);
    } else {
        return body->type_of(program, env);
    }
}

fast::Expr* ast::Block::type_of(fast::Program& program, TypeEnv& env) const {
    if (stmts.size() > 0) {
        std::span<fast::Stmt*> typed_stmts = program.stmts(stmts.size());
        auto typed_body = type_block(program, env, stmts, 0, body, typed_stmts);
        return program.block(span, typed_body->type, std::move(typed_stmts), typed_body);
    } else {
        return body->type_of(program, env);
    }
}

fast::Expr* ast::If::type_of(fast::Program& program, TypeEnv& env) const {
    fast::Expr* const typed_cond = cond->check(program, env, env.types().get_bool());
    fast::Expr* const typed_conseq = conseq->type_of(program, env);
    type::Type* const type = typed_conseq->type;
    fast::Expr* const typed_alt = alt->check(program, env, type); // TODO: treat branch types equally
    return program.if_(span, type, typed_cond, typed_conseq, typed_alt);
}

fast::Expr* ast::Call::type_of(fast::Program &program, TypeEnv &env) const {
    fast::Expr* const typed_callee = callee->type_of(program, env);

    type::FnType* const callee_type = dynamic_cast<type::FnType*>(typed_callee->type); // OPTIMIZE
    if (!callee_type) { throw type::Error(span); }

    if (args.size() != callee_type->domain.size()) { throw type::Error(span); }
    std::size_t arity = callee_type->domain.size();
    std::span<fast::Expr*> typed_args = program.args(arity);
    for (std::size_t i = 0; i < arity; ++i) {
        typed_args[i] = args[i]->check(program, env, callee_type->domain[i]);
    }

    return program.call(span, callee_type->codomain, typed_callee, typed_args);
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

    case ast::PrimApp::Op::EQ_I64: {
        // FIXME: Brittle '2':s:
        if (args.size() != 2) { throw type::Error(span); }

        std::array<fast::Expr*, 2> typed_args;

        for (std::size_t i = 0; i < 2; ++i) {
            typed_args[i] = args[i]->check(program, env, env.types().get_i64());
        }

        return program.eq_i64(span, env.types().get_bool(), typed_args);
    }

    default: assert(false); // unreachable
    }
}

fast::Expr* ast::Bool::type_of(fast::Program& program, TypeEnv& env) const {
    return program.const_bool(span, env.types().get_bool(), value);
}

fast::Expr* ast::Int::type_of(fast::Program& program, TypeEnv& env) const {
    return program.const_i64(span, env.types().get_i64(), digits, strlen(digits));
}

fast::Expr* ast::Id::type_of(fast::Program& program, TypeEnv& env) const {
    std::optional<std::pair<Name, type::Type*>> opt_binder = env.find(name);
    if (opt_binder) {
        return program.id(span, opt_binder->second, opt_binder->first);
    } else {
        throw type::Error(span);
    }
}

fast::Expr* ast::Expr::check(fast::Program& program, TypeEnv& env, type::Type* type) const {
    fast::Expr* expr = type_of(program, env);
    expr->type->unify(type, span);
    return expr;
}

// # Patterns

fast::Pat* ast::Pat::check(fast::Program& program, TypeEnv& env, type::Type* type) const {
    auto const pat = type_of(program, env);
    pat->type->unify(type, span);
    return pat;
}

fast::Pat* ast::IdPat::type_of(fast::Program& program, TypeEnv& env) const {
    auto const type = env.uv();
    Name const unique_name = env.declare(name, type);
    return program.id_pat(span, type, unique_name);
}

fast::Pat* ast::AnnPat::type_of(fast::Program& program, TypeEnv& env) const {
    return pat->check(program, env, type);
}

// # Statements

std::pair<fast::Stmt*, TypeEnv> ast::Val::check(fast::Program& program, TypeEnv& parent_env) const {
    TypeEnv env = parent_env.push_frame();

    fast::Expr* const typed_val_expr = val_expr->type_of(program, env);
    auto const typed_pat = pat->check(program, env, typed_val_expr->type);

    auto const stmt = program.val(span, typed_pat, typed_val_expr);
    return {stmt, std::move(env)};
}

// # Unification

void type::Type::unify(Type* other, Span span) {
    Type* found_this = find();
    Type* found_other = other->find();

    if (found_this != found_other) {
        found_this->unifyFounds(found_other, span);
    }
}

void type::Uv::unifyFoundUvs(type::Uv* uv, Span) {
    uv->union_(this);
}

void type::Type::unifyFoundUvs(type::Uv* uv, Span span) {
    occurs_check(uv, span);
    uv->set(this);
}

void type::Uv::unifyFoundFns(FnType* other, Span span) { other->unifyFoundUvs(this, span); }

void type::Uv::unifyFoundBools(Bool* other, Span span) { other->unifyFoundUvs(this, span); }

void type::Uv::unifyFoundI64s(I64* other, Span span) { other->unifyFoundUvs(this, span); }

void type::FnType::unifyFoundFns(type::FnType* other, Span span) {
    auto dom = domain.begin();
    auto other_dom = other->domain.begin();
    for (;; ++dom, ++other_dom) {
        if (dom != domain.end()) {
            if (other_dom != other->domain.end()) {
                (*other_dom)->unify(*dom, span);
            } else {
                throw type::Error(span);
            }
        } else {
            if (other_dom == other->domain.end()) {
                break;
            } else {
                throw type::Error(span);
            }
        }
    }

    codomain->unify(other->codomain, span);
}

void type::Type::unifyFoundFns(type::FnType*, Span span) { throw type::Error(span); }

void type::Bool::unifyFoundBools(Bool*, Span) {}

void type::Type::unifyFoundBools(Bool*, Span span) { throw type::Error(span); }

void type::I64::unifyFoundI64s(I64*, Span) {}

void type::Type::unifyFoundI64s(I64*, Span span) { throw type::Error(span); }

// ## Occurs Check

void type::Type::occurs_check(Uv *uv, Span span) const {
    if (this == uv) {
        throw type::Error(span);
    } else {
        occurs_check_children(uv, span);
    }
}

void type::Uv::occurs_check_children(Uv* uv, Span span) const {
    parent_.iter([&] (Type const* parent) {
        parent->occurs_check(uv, span);
    });
}

void type::FnType::occurs_check_children(Uv* uv, Span span) const {
    for (Type const* dom : domain) {
        dom->occurs_check(uv, span);
    }

    codomain->occurs_check(uv, span);
}

void type::Type::occurs_check_children(Uv*, Span) const {}

} // namespace brmh
