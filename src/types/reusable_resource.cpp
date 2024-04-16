#include <cassert>
#include "reusable_resource.hpp"
#include "solver.hpp"

namespace ratio
{
    reusable_resource::reusable_resource(solver &slv) : smart_type(slv, "ReusableResource") {}

    void reusable_resource::new_atom(std::shared_ptr<ratio::atom> &atm) noexcept
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