#include <cassert>
#include "state_variable.hpp"
#include "solver.hpp"

namespace ratio
{
    state_variable::state_variable(solver &slv) : smart_type(slv, "StateVariable") {}

    void state_variable::new_atom(std::shared_ptr<ratio::atom> &atm) noexcept
    {
        if (atm->is_fact())
        {
            assert(is_interval(*atm));
            set_ni(atm->get_sigma());
            get_solver().get_predicate("Interval")->get().call(atm);
            restore_ni();
        }
        atoms.push_back(*atm);
    }
} // namespace ratio