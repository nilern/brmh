#include <cstring>

#include "fast.hpp"
#include "cps/cps.hpp"

namespace brmh {

fast::ToCpsNextCont::ToCpsNextCont() : ToCpsCont() {}

bool fast::ToCpsNextCont::is_trivial() const { return false; }

cps::Expr* fast::ToCpsNextCont::operator()(cps::Builder&, Span, cps::Expr* v) const {
    return v;
}

cps::Expr* fast::ToCpsNextCont::call_to(cps::Builder& builder, Call const* fast_call, std::span<cps::Expr*> cps_exprs) const {
    cps::Block* const ret = builder.block(1, nullptr);
    cps::Expr* const result = builder.param(fast_call->span, fast_call->type, ret, builder.names()->fresh(), 0);

    builder.current_block().unwrap()->transfer = builder.call(fast_call->span, cps_exprs, ret);

    builder.set_current_block(ret);
    return result;
}

fast::ToCpsTrivialCont::ToCpsTrivialCont(cps::Cont* cont_) : ToCpsCont(), cont(cont_) {}

bool fast::ToCpsTrivialCont::is_trivial() const { return true; }

cps::Expr* fast::ToCpsTrivialCont::operator()(cps::Builder& builder, Span span, cps::Expr* v) const {
    builder.current_block().unwrap()->transfer = builder.goto_(span, cont, v);
    return v;
}

cps::Expr* fast::ToCpsTrivialCont::call_to(cps::Builder& builder, Call const* fast_call, std::span<cps::Expr*> cps_exprs) const {
    builder.current_block().unwrap()->transfer = builder.call(fast_call->span, cps_exprs, cont);
    return nullptr; // Will not be used
}


cps::Expr* fast::If::to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const {
    cps::Expr* const cps_cond = cond->to_cps(builder, fn, ToCpsNextCont());

    cps::Block* const conseq_block = builder.block(0, nullptr);
    cps::Block* const alt_block = builder.block(0, nullptr);
    builder.current_block().unwrap()->transfer = builder.if_(span, cps_cond, conseq_block, alt_block);

    // TODO: DRY:
    if (k.is_trivial()) {
        builder.set_current_block(conseq_block);
        conseq->to_cps(builder, fn, k);

        builder.set_current_block(alt_block);
        alt->to_cps(builder, fn, k);

        return nullptr; // Will not be used
    } else {
        cps::Block* const join = builder.block(1, nullptr);
        cps::Expr* result = builder.param(span, type, join, builder.names()->fresh(), 0);
        fast::ToCpsTrivialCont join_k(join);

        builder.set_current_block(conseq_block);
        conseq->to_cps(builder, fn, join_k);

        builder.set_current_block(alt_block);
        alt->to_cps(builder, fn, join_k);

        builder.set_current_block(join);
        return result;
    }
}

cps::Expr* fast::Call::to_cps(cps::Builder &builder, cps::Fn *fn, const ToCpsCont &k) const {
    std::span<cps::Expr*> cps_exprs = builder.args(1 + args.size());

    cps_exprs[0] = callee->to_cps(builder, fn, k);

    for (std::size_t i = 0; i < args.size(); ++i) {
        cps_exprs[i + 1] = args[i]->to_cps(builder, fn, ToCpsNextCont());
    }

    return k.call_to(builder, this, cps_exprs);
}

// TODO: DRY:

cps::Expr* fast::AddWI64::to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const {
    // FIXME: brittle '2':s:

    std::array<cps::Expr*, 2> cps_args;

    for (std::size_t i = 0; i < 2; ++i) {
        cps_args[i] = args[i]->to_cps(builder, fn, ToCpsNextCont());
    }

    return k(builder, span, builder.add_w_i64(span, type, cps_args));
}

cps::Expr* fast::SubWI64::to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const {
    // FIXME: brittle '2':s:

    std::array<cps::Expr*, 2> cps_args;

    for (std::size_t i = 0; i < 2; ++i) {
        cps_args[i] = args[i]->to_cps(builder, fn, ToCpsNextCont());
    }

    return k(builder, span, builder.sub_w_i64(span, type, cps_args));
}

cps::Expr* fast::MulWI64::to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const {
    // FIXME: brittle '2':s:

    std::array<cps::Expr*, 2> cps_args;

    for (std::size_t i = 0; i < 2; ++i) {
        cps_args[i] = args[i]->to_cps(builder, fn, ToCpsNextCont());
    }

    return k(builder, span, builder.mul_w_i64(span, type, cps_args));
}

cps::Expr* fast::EqI64::to_cps(cps::Builder& builder, cps::Fn* fn, ToCpsCont const& k) const {
    // FIXME: brittle '2':s:

    std::array<cps::Expr*, 2> cps_args;

    for (std::size_t i = 0; i < 2; ++i) {
        cps_args[i] = args[i]->to_cps(builder, fn, ToCpsNextCont());
    }

    return k(builder, span, builder.eq_i64(span, type, cps_args));
}

cps::Expr* fast::Id::to_cps(cps::Builder& builder, cps::Fn*, ToCpsCont const& k) const {
    return k(builder, span, builder.id(name));
}

cps::Expr* fast::Bool::to_cps(cps::Builder& builder, cps::Fn*, ToCpsCont const& k) const {
    return k(builder, span, builder.const_bool(span, type, value));
}

cps::Expr* fast::I64::to_cps(cps::Builder& builder, cps::Fn*, ToCpsCont const& k) const {
    return k(builder, span, builder.const_i64(span, type, digits, /* OPTIMIZE: */ strlen(digits)));
}

void fast::FunDef::cps_declare(cps::Builder& builder) const {
    type::FnType* const type = builder.types().fn(domain(), codomain);
    cps::Return* const ret = builder.return_(builder.names()->fresh());
    builder.fn(span, name, type, /* FIXME: */ true, ret, nullptr);
}

void fast::FunDef::to_cps(cps::Builder& builder) const {
    cps::Fn* const fn = builder.get_fn(name);

    cps::Block* const entry = builder.block(params.size(), nullptr);
    std::size_t i = 0;
    for (const Param& param : params) {
        builder.param(param.span, param.type, entry, param.name, i);
        ++i;
    }
    fn->entry = entry;

    builder.set_current_block(entry);
    body->to_cps(builder, fn, ToCpsTrivialCont(fn->ret));
}

cps::Program fast::Program::to_cps(Names& names, type::Types& types) const {
    cps::Builder builder(&names, types);

    for (const auto def : defs) {
        def->cps_declare(builder);
    }

    for (const auto def : defs) {
        def->to_cps(builder);
    }

    return builder.build();
}

} // namespace brmh
