#include "hossa_doms.hpp"

#include <optional>

namespace brmh::hossa::doms {

using CompactDomTree = std::vector<std::optional<PostIndex>>;

PostIndex intersect(CompactDomTree const& doms, PostIndex finger1, PostIndex finger2) {
    while (finger1 != finger2) {
        while (finger1 < finger2) {
            finger1 = doms[finger1].value();
        }

        while (finger2 < finger1) {
            finger2 = doms[finger2].value();
        }
    }

    return finger1;
}

DomTree dominator_tree(Fn* fn) {
    // Initialize postorder indices:
    std::vector<Block const*> post_order;
    std::unordered_map<Block const*, PostIndex> block_indices;
    fn->entry->post_visit([&] (Block const* block) {
        std::size_t const i = post_order.size();
        post_order.push_back(block);
        block_indices.insert({block, i});
    });

    // Initialize predecessors:
    std::vector<std::vector<PostIndex>> predecessors(post_order.size());
    for (PostIndex i = 0; i < post_order.size(); ++i) {
        for (Block const* succ : post_order[i]->transfer->successors()) {
            predecessors[block_indices[succ]].push_back(i);
        }
    }

    // Initialize compact dominator tree:
    CompactDomTree doms(post_order.size());
    doms[0] = 0;

    // Control flow analysis:
    bool changed = true;
    while (changed) {
        changed = false;

        for (PostIndex next = post_order.size(); next > 0; --next) {
            PostIndex const i = next - 1;
            std::vector<PostIndex>::const_iterator pred = predecessors[i].cbegin();

            for (; pred != predecessors[i].cend() && !doms[*pred].has_value(); ++pred) {}

            if (pred != predecessors[i].cend()) {
                std::size_t new_idom = *pred;
                for (; pred != predecessors[i].cend(); ++pred) {
                    if (doms[*pred].has_value()) {
                        new_idom = intersect(doms, *pred, new_idom);
                    }
                }

                if (doms[i] != new_idom) {
                    doms[i] = new_idom;
                    changed = true;
                }
            }
        }
    }

    // Expand dominator tree:
    DomTree res;
    for (PostIndex i = 0; i < doms.size(); ++i) {
        res.insert({post_order[i], {i, post_order[doms[i].value()]}});
    }
    return res;
}

Block const* lca(DomTree const& doms, Block const* block1, Block const* block2) {
    DomTreeNode node1 = doms.at(block1);
    DomTreeNode node2 = doms.at(block2);

    while (node1.post_index != node2.post_index) {
        while (node1.post_index < node2.post_index) {
            block1 = node1.parent;
            node1 = doms.at(block1);
        }

        while (node2.post_index < node1.post_index) {
            block2 = node2.parent;
            node2 = doms.at(block2);
        }
    }

    return block1;
}

}
