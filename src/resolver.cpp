#include "resolver.h"
#include "flaw.h"
#include "solver.h"
#include <cassert>

namespace ratio
{
    resolver::resolver(flaw &f, const utils::rational &cost) : resolver(f, semitone::lit(f.s.sat->new_var()), cost) {}
    resolver::resolver(flaw &f, const semitone::lit &rho, const utils::rational &cost) : f(f), rho(rho), intrinsic_cost(cost) { assert(f.s.get_sat_core().value(rho) != utils::False); }

    utils::rational resolver::get_estimated_cost() const noexcept
    {
        if (f.s.get_sat_core().value(rho) == utils::False)
            return utils::rational::POSITIVE_INFINITY;
        else if (preconditions.empty())
            return intrinsic_cost;

        utils::rational est_cost;
#ifdef H_MAX
        est_cost = utils::rational::NEGATIVE_INFINITY;
        for (const auto &p : preconditions)
            if (!p.get().is_expanded())
                return utils::rational::POSITIVE_INFINITY;
            else // we compute the max of the flaws' estimated costs..
                est_cost = std::max(est_cost, p.get().get_estimated_cost());
#endif
#ifdef H_ADD
        est_cost = utils::rational::ZERO;
        for (const auto &p : preconditions)
            if (!p.get().is_expanded())
                return utils::rational::POSITIVE_INFINITY;
            else // we compute the sum of the flaws' estimated costs..
                est_cost += p.get().get_estimated_cost();
#endif
        return est_cost + intrinsic_cost;
    }
} // namespace ratio