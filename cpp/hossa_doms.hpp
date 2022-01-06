#ifndef BRMH_HOSSA_DOMS_HPP
#define BRMH_HOSSA_DOMS_HPP

#include <unordered_map>

#include "hossa.hpp"

namespace brmh::hossa::doms {

using DomTree = std::unordered_map<Block const*, Block const*>;

DomTree dominator_tree(Fn* fn);

Block const* lca(DomTree const& doms, Block const*, Block const*);

}

#endif
