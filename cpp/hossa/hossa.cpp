#include "hossa.hpp"
#include "schedule.hpp"

#include <cstring>

namespace brmh::hossa {

void Fn::print_def(Names& names, std::ostream& dest) const {
    doms::DomTree const doms = doms::DomTree::of(this);
    schedule::Schedule const schedule = schedule::schedule_late(this, doms);

    std::unordered_set<Block const*> visited_blocks;
    std::unordered_set<Expr const*> visited_exprs;

    dest << "fun ";
    name.print(names, dest);

    dest << " -> ";

    static_cast<type::FnType*>(type)->codomain->print(names, dest); // HACK: static_cast

    dest << " {" << std::endl;

    dest << "    ";
    entry->print(names, dest, schedule, visited_blocks, visited_exprs);

    dest << std::endl << '}';
}

void Block::print(Names& names, std::ostream& dest, schedule::Schedule const& schedule,
                  std::unordered_set<Block const*>& visited_blocks,
                  std::unordered_set<Expr const*>& visited_exprs) const {
    if (!visited_blocks.contains(this)) {
        visited_blocks.insert(this);
        name.print(names, dest);

        dest << " (";

        auto param = params.begin();
        if (param != params.end()) {
            (*param)->name.print(names, dest);
            dest << " : ";
            (*param)->type->print(names, dest);
            ++param;

            for (; param != params.end(); ++param) {
                dest << ", ";
                (*param)->name.print(names, dest);
                dest << " : ";
                (*param)->type->print(names, dest);
            }
        }

        dest << "):" << std::endl;

        transfer->print(names, dest, schedule, visited_blocks, visited_exprs, this);
    }
}

void Transfer::print(Names& names, std::ostream& dest, schedule::Schedule const& schedule,
                     std::unordered_set<Block const*>& visited_blocks, std::unordered_set<Expr const*>& visited_exprs,
                     Block const* block) const {
    for (Expr const* operand : operands()) {
        operand->print_in(names, dest, schedule, visited_exprs, block);
    }

    do_print(names, dest);

    for (Block const* succ : successors()) {
        dest << "\n\n    ";
        succ->print(names, dest, schedule, visited_blocks, visited_exprs);
    }
}

void If::do_print(Names& names, std::ostream& dest) const {
    dest << "        if ";
    cond->name.print(names, dest);
    dest << "\n        then goto ";
    conseq->name.print(names, dest);
    dest << "\n        else goto ";
    alt->name.print(names, dest);
}

void Goto::do_print(Names& names, std::ostream& desto) const {
    desto << "        goto ";
    dest->name.print(names, desto);
    desto << ' ';
    res->name.print(names, desto);
}

void Expr::print_in(Names& names, std::ostream& dest, schedule::Schedule const& schedule,
                    std::unordered_set<Expr const*>& visited_exprs, Block const* block) const {
    if (schedule.at(this) == block) {
        if (!visited_exprs.contains(this)) {
            visited_exprs.insert(this);

            for (Expr const* operand : operands()) {
                operand->print_in(names, dest, schedule, visited_exprs, block);
            }

            dest << "        ";
            name.print(names, dest);
            dest << " : ";
            type->print(names, dest);
            dest << " = ";
            do_print(names, dest);
            dest << "\n";
        }
    }
}

} // namespace brmh::hossa
