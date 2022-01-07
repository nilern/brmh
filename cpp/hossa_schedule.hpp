#ifndef BRMH_HOSSA_SCHEDULE_HPP
#define BRMH_HOSSA_SCHEDULE_HPP

#include <unordered_map>

#include "hossa.hpp"

namespace brmh::hossa::schedule {

using Schedule = std::unordered_map<Expr const*, Block const*>;

Schedule schedule_late(Fn const* fn);

}

#endif // BRMH_HOSSA_SCHEDULE_SCHEDULER_HPP
