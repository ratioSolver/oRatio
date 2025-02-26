#include "sttypes.hpp"
#include "stsolver.hpp"
#include <sstream>

namespace ratio
{
    ststate_variable::ststate_variable(stsolver &slv) noexcept : state_variable(slv) {}

    bool ststate_variable::solve_inconsistencies() {}

    streusable_resource::streusable_resource(stsolver &slv) noexcept : reusable_resource(slv) {}

    bool streusable_resource::solve_inconsistencies() {}

    stconsumable_resource::stconsumable_resource(stsolver &slv) noexcept : consumable_resource(slv) {}

    bool stconsumable_resource::solve_inconsistencies() {}
} // namespace ratio
