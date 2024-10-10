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
    resolver::resolver(flaw &f, const utils::lit &rho, const utils::rational &intrinsic_cost) : f(f), rho(rho), intrinsic_cost(intrinsic_cost)
    {
        assert(f.get_solver().get_sat().value(rho) != utils::False);
#ifdef ENABLE_API
        f.get_solver().get_sat().add_listener(*this);

        listen_sat(variable(rho));
#endif
    }

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

#ifdef ENABLE_API
    void resolver::on_sat_value_changed(VARIABLE_TYPE v) { f.get_solver().resolver_state_changed(*this); }
#endif
} // namespace ratio
