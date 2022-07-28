#include "resolver.h"
#include "flaw.h"
#include "causal_graph.h"
#include "solver.h"

namespace ratio::solver
{
    resolver::resolver(semitone::rational cost, flaw &eff) : resolver(semitone::lit(eff.get_solver().get_sat()->new_var()), std::move(cost), eff) {}
    resolver::resolver(semitone::lit r, semitone::rational cost, flaw &eff) : slv(eff.get_solver()), rho(std::move(r)), intrinsic_cost(std::move(cost)), effect(eff) {}

    ORATIO_EXPORT semitone::rational resolver::get_estimated_cost() const noexcept { return slv.get_causal_graph().get_estimated_cost(*this); }
} // namespace ratio::solver
