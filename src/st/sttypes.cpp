#include "sttypes.hpp"
#include "stsolver.hpp"
#include <sstream>

namespace ratio
{
    statom_listener::statom_listener(atom &atm) noexcept : prop_listener(static_cast<solver &>(atm.get_core())), la_listener(static_cast<solver &>(atm.get_core()).get_linear_arithmetic_theory())
    {
    }
    void statom_listener::on_change(const utils::var &v) noexcept
    {
    }
    void statom_listener::on_arith_change(const utils::var &v) noexcept
    {
    }

    ststate_variable::ststate_variable(solver &slv) noexcept : state_variable(slv) {}

    bool ststate_variable::solve_inconsistencies() { return false; }

    streusable_resource::streusable_resource(solver &slv) noexcept : reusable_resource(slv) {}

    bool streusable_resource::solve_inconsistencies() { return false; }

    stconsumable_resource::stconsumable_resource(solver &slv) noexcept : consumable_resource(slv) {}

    bool stconsumable_resource::solve_inconsistencies() { return false; }
} // namespace ratio
