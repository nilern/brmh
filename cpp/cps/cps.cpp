#include "cps.hpp"
#include "schedule.hpp"

#include <cstring>

namespace brmh::cps {

void Fn::print_def(Names const& names, std::ostream& dest) const {
    doms::DomTree const doms = doms::DomTree::of(this);

    schedule::Schedule const schedule = schedule::schedule_late(this, doms);
    std::unordered_map<Block const*, std::vector<Expr const*>> block_exprs;
    for (auto expr_block : schedule) {
        Expr const* const expr = expr_block.first;
        Block const* const block = expr_block.second;
        auto it = block_exprs.find(block);
        if (it != block_exprs.end()) {
            it->second.push_back(expr);
        } else {
            block_exprs.insert({block, {expr}});
        }
    }

    std::unordered_set<Expr const*> visited_exprs;

    PrintCtx ctx(names, std::move(block_exprs), std::move(visited_exprs));

    dest << "fun ";
    name.print(names, dest);

    dest << ' ';

    ret->name.print(names, dest);

    dest << " -> ";

    ret->name.print(names, dest);

    dest << '(';

    static_cast<type::FnType*>(type)->codomain->print(names, dest); // HACK: static_cast

    dest << ") {" << std::endl;

    doms.pre_visit_blocks([&] (Block const* block) {
        block->print(ctx, dest);
        dest << "\n\n";
    });

    dest << std::endl << '}';
}

void Call::do_print(Names const& names, std::ostream& dest) const {
    dest << "        call ";
    callee()->name.print(names, dest);

    dest << '(';

    auto arg = args().begin();
    if (arg != args().end()) {
        (*arg)->name.print(names, dest);
        ++arg;

        for (; arg != args().end(); ++arg) {
            dest << ", ";
            (*arg)->name.print(names, dest);
        }
    }

    dest << ") -> ";

    cont->name.print(names, dest);
}

void Block::print(PrintCtx& ctx, std::ostream& dest) const {
    dest << "    ";
    name.print(ctx.names, dest);

    dest << " (";

    auto param = params.begin();
    if (param != params.end()) {
        (*param)->name.print(ctx.names, dest);
        dest << " : ";
        (*param)->type->print(ctx.names, dest);
        ++param;

        for (; param != params.end(); ++param) {
            dest << ", ";
            (*param)->name.print(ctx.names, dest);
            dest << " : ";
            (*param)->type->print(ctx.names, dest);
        }
    }

    dest << "):" << std::endl;

    for (Expr const* expr : ctx.block_exprs.at(this)) {
        expr->print_in(ctx, dest);
    }

    transfer->do_print(ctx.names, dest);
}

void If::do_print(Names const& names, std::ostream& dest) const {
    dest << "        if ";
    cond->name.print(names, dest);
    dest << "\n        then goto ";
    conseq->name.print(names, dest);
    dest << "()\n        else goto ";
    alt->name.print(names, dest);
    dest << "()";
}

void Goto::do_print(Names const& names, std::ostream& desto) const {
    desto << "        goto ";
    dest->name.print(names, desto);
    desto << '(';
    res->name.print(names, desto);
    desto << ')';
}

void Expr::print_in(PrintCtx& ctx, std::ostream& dest) const {
    if (!ctx.visited_exprs.contains(this)) {
        ctx.visited_exprs.insert(this);

        for (Expr const* operand : operands()) {
            operand->print_in(ctx, dest);
        }

        dest << "        ";
        name.print(ctx.names, dest);
        dest << " : ";
        type->print(ctx.names, dest);
        dest << " = ";
        do_print(ctx.names, dest);
        dest << "\n";
    }
}

} // namespace brmh::cps
