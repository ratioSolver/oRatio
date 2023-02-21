#include "flaw.h"
#include "solver.h"
#include "resolver.h"
#include "sat_core.h"

namespace ratio
{
    flaw::flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, const bool &exclusive) : s(s), causes(causes), exclusive(exclusive)
    {
    }

    ORATIOSOLVER_EXPORT void flaw::add_resolver(utils::u_ptr<resolver> r)
    {
        if (!s.get_sat_core().new_clause({!r->get_rho(), phi}))
            throw riddle::unsolvable_exception();
        resolvers.push_back(*r);
        s.new_resolver(std::move(r)); // we notify the solver that a new resolver has been added..
    }
} // namespace ratio
