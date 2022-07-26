#include "flaw.h"
#include "resolver.h"
#include "causal_graph.h"
#include "solver.h"

namespace ratio::solver
{
    ORATIO_EXPORT flaw::flaw(causal_graph &gr, std::vector<resolver *> causes, const bool &exclusive) : gr(gr), position(gr.get_solver().get_idl_theory().new_var()), causes(std::move(causes)), exclusive(exclusive) {}

    ORATIO_EXPORT resolver *flaw::get_cheapest_resolver() const noexcept
    {
        resolver *c_res = nullptr;
        semitone::rational c_cost = semitone::rational::POSITIVE_INFINITY;
        for (const auto &r : resolvers)
            if (gr.get_solver().get_sat_core().value(r->get_rho()) != semitone::False && r->get_estimated_cost() < c_cost)
            {
                c_res = r;
                c_cost = r->get_estimated_cost();
            }
        return c_res;
    }
} // namespace ratio::solver
