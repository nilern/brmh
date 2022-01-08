#ifndef BRMH_HOSSA_DOMS_HPP
#define BRMH_HOSSA_DOMS_HPP

#include <unordered_map>

#include "util.hpp"
#include "hossa.hpp"

namespace brmh::hossa::doms {

using PostIndex = std::size_t;

class DomTreeBuilder;

struct DomTreeNode {
    PostIndex post_index; // Postorder number
    opt_ptr<DomTreeNode> parent; // Immediate dominator

    DomTreeNode(PostIndex post_index_, opt_ptr<DomTreeNode> parent_)
        : post_index(post_index_), parent(parent_) {}

private:
    friend class Dominators;
};

class DomTree {
    BumpArena arena_;
public:
    std::unordered_map<Block const*, DomTreeNode*> block_nodes;

private:
    friend class DomTreeBuilder;

    DomTree(BumpArena&& arena, std::unordered_map<Block const*, DomTreeNode*>&& block_nodes_)
        : arena_(std::move(arena)), block_nodes(std::move(block_nodes_)) {}
};

class DomTreeBuilder {
    BumpArena arena_;
    std::unordered_map<Block const*, DomTreeNode*> block_nodes_;

public:
    void node(Block const* block, PostIndex post_index, opt_ptr<Block const> opt_parent_block) {
        opt_ptr<DomTreeNode> parent = opt_parent_block.map<DomTreeNode>([&] (Block const* parent_block) {
            return block_nodes_.at(parent_block);
        });
        auto node = new (arena_.alloc<DomTreeNode>()) DomTreeNode(post_index, parent);
        block_nodes_.insert({block, node});
    }

    DomTree build() { return DomTree(std::move(arena_), std::move(block_nodes_)); }
};

DomTree dominator_tree(Fn const* fn);

Block const* lca(DomTree const& doms, Block const* block1, Block const* block2);

}

#endif
