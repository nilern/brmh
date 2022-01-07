#ifndef BRMH_HOSSA_DOMS_HPP
#define BRMH_HOSSA_DOMS_HPP

#include <unordered_map>

#include "hossa.hpp"

namespace brmh::hossa::doms {

using PostIndex = std::size_t;

struct DomTreeNode {
    PostIndex post_index; // Postorder number
    Block const* parent; // Immediate dominator
};

using DomTree = std::unordered_map<Block const*, DomTreeNode>;

DomTree dominator_tree(Fn* fn);

Block const* lca(DomTree const& doms, Block const* block1, Block const* block2);

}

#endif
