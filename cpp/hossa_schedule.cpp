#include "hossa_schedule.hpp"

#include "hossa_doms.hpp"

namespace brmh::hossa::schedule {

struct SetupVisitor : public hossa::TransfersExprsVisitor {
    std::vector<Expr const*> post_order;
    std::unordered_map<Expr const*, std::vector<Expr const*>> use_exprs;
    std::unordered_map<Expr const*, std::vector<Transfer const*>> use_transfers;

    SetupVisitor() : post_order() {}

    virtual void visit(Transfer const* transfer) override {
        for (Expr const* arg : transfer->operands()) {
            auto it = use_transfers.find(arg);
            if (it != use_transfers.end()) {
                it->second.push_back(transfer);
            } else {
                use_transfers.insert({arg, {transfer}});
            }
        }
    }

    virtual void visit(Expr const* expr) override {
        post_order.push_back(expr);

        for (Expr const* arg : expr->operands()) {
            auto it = use_exprs.find(arg);
            if (it != use_exprs.end()) {
                it->second.push_back(expr);
            } else {
                use_exprs.insert({arg, {expr}});
            }
        }
    }
};

Schedule schedule_late(Fn *fn) {
    // Initialize postorder and reverse mappings:

    SetupVisitor visitor;
    fn->post_visit_transfers_and_exprs(visitor);
    std::vector<Expr const*> post_order = std::move(visitor.post_order);
    std::unordered_map<Expr const*, std::vector<Expr const*>> use_exprs = std::move(visitor.use_exprs);
    std::unordered_map<Expr const*, std::vector<Transfer const*>> use_transfers = std::move(visitor.use_transfers);

    std::unordered_map<Transfer const*, Block const*> transfer_blocks;
    fn->entry->post_visit([&] (Block const* block) {
        transfer_blocks.insert({block->transfer, block});
    });

    // Compute dominator tree:
    doms::DomTree const doms = doms::dominator_tree(fn);

    // Schedule in reverse postorder:
    Schedule res;
    std::for_each(post_order.crbegin(), post_order.crend(), [&] (Expr const* expr) {
        Block const* parent = nullptr;

        for (Expr const* use : use_exprs.at(expr)) {
            Block const* const use_parent = res.at(use);
            if (parent == nullptr) {
                parent = use_parent;
            } else {
                parent = doms::lca(doms, parent, use_parent);
            }
        }

        for (Transfer const* use : use_transfers.at(expr)) {
            Block const* const use_parent = transfer_blocks.at(use);
            if (parent == nullptr) {
                parent = use_parent;
            } else {
                parent = doms::lca(doms, parent, use_parent);
            }
        }
    });

    return res;
}

}
