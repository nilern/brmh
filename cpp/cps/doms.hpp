#ifndef BRMH_HOSSA_DOMS_HPP
#define BRMH_HOSSA_DOMS_HPP

#include <unordered_map>

#include "../util.hpp"
#include "cps.hpp"

namespace brmh::cps::doms {

using PostIndex = std::size_t;

class DomTreeBuilder;

struct DomTreeNode {
    Block const* block;
    PostIndex post_index; // Postorder number
    opt_ptr<DomTreeNode> parent; // Immediate dominator

private:
    friend class DomTree;
    friend class DomTreeBuilder;

    DomTreeNode(Block const* block_, PostIndex post_index_, opt_ptr<DomTreeNode> parent_)
        : block(block_), post_index(post_index_), parent(parent_) {}

    template<typename F>
    void do_pre_visit(F f, std::unordered_set<DomTreeNode const*>& visited) const {
        if (!visited.contains(this)) {
            visited.insert(this);
            parent.iter([&] (DomTreeNode const* parent) { parent->do_pre_visit(f, visited); });
            f(this->block);
        }
    }
};

class DomTree {
    BumpArena arena_;
public:
    std::unordered_map<Block const*, DomTreeNode*> block_nodes;

private:
    friend class DomTreeBuilder;

    DomTree(BumpArena&& arena, std::unordered_map<Block const*, DomTreeNode*>&& block_nodes_)
        : arena_(std::move(arena)), block_nodes(std::move(block_nodes_)) {}

public:
    static DomTree of(Fn const* fn);

    template<typename F>
    void pre_visit_blocks(F f) const {
        std::unordered_set<DomTreeNode const*> visited;
        for (auto kv : block_nodes) {
            kv.second->do_pre_visit(f, visited);
        }
    }

    Block const* lca(Block const* block1, Block const* block2) const;
};

class DomTreeBuilder {
    BumpArena arena_;
    std::unordered_map<Block const*, DomTreeNode*> block_nodes_;

public:
    void node(Block const* block, PostIndex post_index, opt_ptr<Block const> opt_parent_block) {
        opt_ptr<DomTreeNode> parent = opt_parent_block.map<DomTreeNode>([&] (Block const* parent_block) {
            return block_nodes_.at(parent_block);
        });
        auto node = new (arena_.alloc<DomTreeNode>()) DomTreeNode(block, post_index, parent);
        block_nodes_.insert({block, node});
    }

    DomTree build() { return DomTree(std::move(arena_), std::move(block_nodes_)); }
};

}

#endif
