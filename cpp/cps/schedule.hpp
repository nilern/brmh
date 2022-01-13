#ifndef BRMH_HOSSA_SCHEDULE_HPP
#define BRMH_HOSSA_SCHEDULE_HPP

#include <unordered_map>

#include "cps.hpp"
#include "doms.hpp"

namespace brmh::cps::schedule {

using Schedule = std::unordered_map<Expr const*, Block const*>;

Schedule schedule_late(Fn const* fn, doms::DomTree const& doms);

}

#endif // BRMH_HOSSA_SCHEDULE_SCHEDULER_HPP
