#include <cstring>

#include "fast.hpp"
#include "hossa.hpp"

namespace brmh {

fast::ToHossaNextCont::ToHossaNextCont() : ToHossaCont() {}

bool fast::ToHossaNextCont::is_tail() const { return false; }

bool fast::ToHossaNextCont::is_trivial() const { return false; }

hossa::Expr* fast::ToHossaNextCont::operator()(hossa::Builder&, Span, hossa::Expr* v) const {
    return v;
}

fast::ToHossaLabelCont::ToHossaLabelCont(hossa::Block* block_) : ToHossaCont(), block(block_) {}

bool fast::ToHossaLabelCont::is_tail() const { return false; }

bool fast::ToHossaLabelCont::is_trivial() const { return true; }

hossa::Expr* fast::ToHossaLabelCont::operator()(hossa::Builder& builder, Span span, hossa::Expr* v) const {
    builder.current_block().unwrap()->transfer = builder.goto_(span, block, v);
    return v;
}

fast::ToHossaReturnCont::ToHossaReturnCont() : ToHossaCont() {}

bool fast::ToHossaReturnCont::is_tail() const { return true; }

bool fast::ToHossaReturnCont::is_trivial() const { return true; }

hossa::Expr* fast::ToHossaReturnCont::operator()(hossa::Builder& builder, Span span, hossa::Expr* v) const {
    builder.current_block().unwrap()->transfer = builder.ret(span, v);
    return v;
}

hossa::Expr* fast::If::to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const {
    hossa::Expr* const hossa_cond = cond->to_hossa(builder, fn, ToHossaNextCont());

    hossa::Block* const conseq_block = builder.block(0, nullptr);
    hossa::Block* const alt_block = builder.block(0, nullptr);
    builder.current_block().unwrap()->transfer = builder.if_(span, hossa_cond, conseq_block, alt_block);

    // TODO: DRY:
    if (k.is_trivial()) {
        builder.set_current_block(conseq_block);
        conseq->to_hossa(builder, fn, k);

        builder.set_current_block(alt_block);
        alt->to_hossa(builder, fn, k);

        return nullptr; // Will not be used
    } else {
        hossa::Block* const join = builder.block(1, nullptr);
        hossa::Expr* result = builder.param(span, type, join, builder.names()->fresh(), 0);
        fast::ToHossaLabelCont join_k(join);

        builder.set_current_block(conseq_block);
        conseq->to_hossa(builder, fn, join_k);

        builder.set_current_block(alt_block);
        alt->to_hossa(builder, fn, join_k);

        builder.set_current_block(join);
        return result;
    }
}

hossa::Expr* fast::Call::to_hossa(hossa::Builder &builder, hossa::Fn *fn, const ToHossaCont &k) const {
    std::span<hossa::Expr*> hossa_exprs = builder.args(1 + args.size());

    hossa_exprs[0] = callee->to_hossa(builder, fn, k);

    for (std::size_t i = 0; i < args.size(); ++i) {
        hossa_exprs[i + 1] = args[i]->to_hossa(builder, fn, ToHossaNextCont());
    }

    if (k.is_tail()) {
        builder.current_block().unwrap()->transfer = builder.tail_call(span, hossa_exprs);
        return nullptr; // Will not be used
    } else {
        hossa::Expr* const hossa_call = builder.call(span, type, hossa_exprs);
        return k(builder, span, hossa_call);
    }
}

// TODO: DRY:

hossa::Expr* fast::AddWI64::to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const {
    // FIXME: brittle '2':s:

    std::array<hossa::Expr*, 2> hossa_args;

    for (std::size_t i = 0; i < 2; ++i) {
        hossa_args[i] = args[i]->to_hossa(builder, fn, ToHossaNextCont());
    }

    return k(builder, span, builder.add_w_i64(span, type, hossa_args));
}

hossa::Expr* fast::SubWI64::to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const {
    // FIXME: brittle '2':s:

    std::array<hossa::Expr*, 2> hossa_args;

    for (std::size_t i = 0; i < 2; ++i) {
        hossa_args[i] = args[i]->to_hossa(builder, fn, ToHossaNextCont());
    }

    return k(builder, span, builder.sub_w_i64(span, type, hossa_args));
}

hossa::Expr* fast::MulWI64::to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const {
    // FIXME: brittle '2':s:

    std::array<hossa::Expr*, 2> hossa_args;

    for (std::size_t i = 0; i < 2; ++i) {
        hossa_args[i] = args[i]->to_hossa(builder, fn, ToHossaNextCont());
    }

    return k(builder, span, builder.mul_w_i64(span, type, hossa_args));
}

hossa::Expr* fast::EqI64::to_hossa(hossa::Builder& builder, hossa::Fn* fn, ToHossaCont const& k) const {
    // FIXME: brittle '2':s:

    std::array<hossa::Expr*, 2> hossa_args;

    for (std::size_t i = 0; i < 2; ++i) {
        hossa_args[i] = args[i]->to_hossa(builder, fn, ToHossaNextCont());
    }

    return k(builder, span, builder.eq_i64(span, type, hossa_args));
}

hossa::Expr* fast::Id::to_hossa(hossa::Builder& builder, hossa::Fn*, ToHossaCont const& k) const {
    return k(builder, span, builder.id(name));
}

hossa::Expr* fast::Bool::to_hossa(hossa::Builder& builder, hossa::Fn*, ToHossaCont const& k) const {
    return k(builder, span, builder.const_bool(span, type, value));
}

hossa::Expr* fast::I64::to_hossa(hossa::Builder& builder, hossa::Fn*, ToHossaCont const& k) const {
    return k(builder, span, builder.const_i64(span, type, digits, /* OPTIMIZE: */ strlen(digits)));
}

void fast::FunDef::hossa_declare(hossa::Builder& builder) const {
    type::FnType* const type = builder.types().fn(domain(), codomain);
    builder.fn(span, name, type, /* FIXME: */ true, nullptr);
}

void fast::FunDef::to_hossa(hossa::Builder& builder) const {
    hossa::Fn* const fn = builder.get_fn(name);

    hossa::Block* const entry = builder.block(params.size(), nullptr);
    std::size_t i = 0;
    for (const Param& param : params) {
        builder.param(param.span, param.type, entry, param.name, i);
        ++i;
    }
    fn->entry = entry;

    builder.set_current_block(entry);
    body->to_hossa(builder, fn, ToHossaReturnCont());
}

hossa::Program fast::Program::to_hossa(Names& names, type::Types& types) const {
    hossa::Builder builder(&names, types);

    for (const auto def : defs) {
        def->hossa_declare(builder);
    }

    for (const auto def : defs) {
        def->to_hossa(builder);
    }

    return builder.build();
}

} // namespace brmh
