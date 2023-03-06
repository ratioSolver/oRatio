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

    std::string to_string(const resolver &r) noexcept
    {
        std::string state;
        switch (r.f.get_solver().get_sat_core().value(r.get_rho()))
        {
        case utils::True: // the resolver is active..
            state = "active";
            break;
        case utils::False: // the resolver is forbidden..
            state = "forbidden";
            break;
        default: // the resolver is inactive..
            state = "inactive";
            break;
        }
        return "ρ" + std::to_string(variable(r.get_rho())) + " (" + state + ")";
    }
} // namespace ratio