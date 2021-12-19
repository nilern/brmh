#include <cstring>

#include "fast.hpp"
#include "hossa.hpp"

namespace brmh {

hossa::Expr* fast::Id::to_hossa(hossa::Builder& builder) const {
    return builder.id(name);
}

hossa::Expr* fast::Int::to_hossa(hossa::Builder& builder) const {
    return builder.const_int(span, type, digits, /* OPTIMIZE: */ strlen(digits));
}

void fast::FunDef::to_hossa(hossa::Builder& builder) const {
    hossa::Fn* const fn = builder.fn(span, name, codomain, /* FIXME: */ true, nullptr);
    hossa::Block* const entry = builder.block(fn, params.size(), nullptr);

    std::size_t i = 0;
    for (const Param& param : params) {
        builder.param(param.span, param.type, entry, param.name, i);
        ++i;
    }

    hossa::Expr* const body_expr = body->to_hossa(builder);
    entry->transfer = builder.ret(body_expr->span, body_expr);

    fn->entry = entry;
}

hossa::Program fast::Program::to_hossa(Names& names) const {
    hossa::Builder builder(&names);

    for (const auto def : defs) {
        def->to_hossa(builder);
    }

    return builder.build();
}

} // namespace brmh
