#include <cassert>
#include "agent.hpp"
#include "solver.hpp"

namespace ratio
{
    agent::agent(solver &slv) : smart_type(slv, "Agent") {}

    void agent::new_atom(std::shared_ptr<ratio::atom> &atm) noexcept
    {
        if (atm->is_fact())
        {
            set_ni(atm->get_sigma());
            if (is_impulse(*atm))
                get_solver().get_predicate("Impulse")->get().call(atm);
            else
                get_solver().get_predicate("Interval")->get().call(atm);
            restore_ni();
        }
        atoms.push_back(*atm);
    }

#ifdef ENABLE_VISUALIZATION
    json::json agent::extract() const noexcept {}
#endif
} // namespace ratio