#include <cassert>
#if defined(H_ADD) || defined(H2_ADD)
#include <numeric>
#endif
#if defined(H_MAX) || defined(H2_MAX)
#include <algorithm>
#endif
#include "resolver.hpp"
#include "flaw.hpp"
#include "solver.hpp"

namespace ratio
{
    resolver::resolver(flaw &f, const utils::rational &intrinsic_cost) : resolver(f, utils::lit(f.get_solver().get_sat().new_var()), intrinsic_cost) {}
    resolver::resolver(flaw &f, const utils::lit &rho, const utils::rational &intrinsic_cost) : f(f), rho(rho), intrinsic_cost(intrinsic_cost) { assert(f.get_solver().get_sat().value(rho) != utils::False); }

    utils::rational resolver::get_estimated_cost() const noexcept
    {
        if (f.get_solver().get_sat().value(rho) == utils::False)
            return utils::rational::positive_infinite;
        else if (preconditions.empty())
            return intrinsic_cost;

#if defined(H_ADD) || defined(H2_ADD)
        return intrinsic_cost + std::accumulate(preconditions.begin(), preconditions.end(), intrinsic_cost, [](const auto &acc, const auto &precondition)
                                                { return acc + precondition.get().get_estimated_cost(); });
#endif
#if defined(H_MAX) || defined(H2_MAX)
        return intrinsic_cost + std::max_element(preconditions.begin(), preconditions.end(), [](const auto &lhs, const auto &rhs)
                                                 { return lhs.get().get_estimated_cost() < rhs.get().get_estimated_cost(); })
                                    ->get()
                                    .get_estimated_cost();
#endif
    }

#ifdef ENABLE_VISUALIZATION
    json::json to_json(const resolver &r) noexcept
    {
        json::json j_r{{"id", get_id(r)}, {"state", to_state(r)}, {"flaw", get_id(r.get_flaw())}, {"rho", to_string(r.get_rho())}, {"intrinsic_cost", to_json(r.get_intrinsic_cost())}, {"data", r.get_data()}};
        json::json preconditions(json::json_type::array);
        for (const auto &p : r.get_preconditions())
            preconditions.push_back(get_id(p.get()));
        j_r["preconditions"] = std::move(preconditions);
        return j_r;
    }

    std::string to_state(const resolver &r) noexcept
    {
        switch (r.get_flaw().get_solver().get_sat().value(r.get_rho()))
        {
        case utils::True:
            return "active";
        case utils::False:
            return "forbidden";
        default:
            return "inactive";
        }
    }
#endif
} // namespace ratio
