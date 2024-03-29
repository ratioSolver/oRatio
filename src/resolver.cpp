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
        if (get_solver().get_sat_core().value(rho) == utils::False)
            return utils::rational::POSITIVE_INFINITY;
        else if (preconditions.empty())
            return intrinsic_cost;

        utils::rational est_cost;
#if defined(H_MAX) || defined(H2_MAX)
        est_cost = utils::rational::NEGATIVE_INFINITY;
        for (const auto &p : preconditions)
            if (!p.get().is_expanded())
                return utils::rational::POSITIVE_INFINITY;
            else // we compute the max of the flaws' estimated costs..
                est_cost = std::max(est_cost, p.get().get_estimated_cost());
#endif
#if defined(H_ADD) || defined(H2_ADD)
        est_cost = utils::rational::ZERO;
        for (const auto &p : preconditions)
            if (!p.get().is_expanded())
                return utils::rational::POSITIVE_INFINITY;
            else // we compute the sum of the flaws' estimated costs..
                est_cost += p.get().get_estimated_cost();
#endif
        return est_cost + intrinsic_cost;
    }

    void resolver::new_causal_link(flaw &f) { f.s.new_causal_link(f, *this); }

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

    json::json to_json(const resolver &r) noexcept
    {
        json::json j_r{{"id", get_id(r)}, {"effect", get_id(r.get_flaw())}, {"rho", to_string(r.get_rho())}, {"state", r.get_solver().get_sat_core().value(r.get_rho())}, {"intrinsic_cost", to_json(r.get_intrinsic_cost())}, {"data", r.get_data()}};
        json::json preconditions(json::json_type::array);
        for (const auto &p : r.get_preconditions())
            preconditions.push_back(get_id(p.get()));
        j_r["preconditions"] = std::move(preconditions);
        return j_r;
    }
} // namespace ratio