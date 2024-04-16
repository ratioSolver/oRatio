#include "agent.hpp"
#include "solver.hpp"

namespace ratio
{
    agent::agent(solver &slv) : smart_type(slv, "agent") {}

    void agent::new_atom(std::shared_ptr<ratio::atom> &atm) noexcept
    {
        if (atm->is_fact())
        {
            set_ni(atm->get_sigma());
            if (is_impulse(*atm))
                atm->get_core().get_predicate("Impulse")->get();
        }
        atoms.push_back(*atm);
    }
} // namespace ratio