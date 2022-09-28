#include "resolver.h"
#include "flaw.h"
#include "causal_graph.h"
#include "solver.h"
#include <cassert>

namespace ratio::solver
{
    resolver::resolver(semitone::rational cost, flaw &eff) : resolver(semitone::lit(eff.get_solver().get_sat_core()->new_var()), std::move(cost), eff) {}
    resolver::resolver(semitone::lit r, semitone::rational cost, flaw &eff) : slv(eff.get_solver()), rho(std::move(r)), intrinsic_cost(std::move(cost)), effect(eff) {}

    ORATIO_EXPORT semitone::rational resolver::get_estimated_cost() const noexcept { return slv.get_causal_graph().get_estimated_cost(*this); }

    ORATIO_EXPORT json::json to_json(const resolver &rhs) noexcept
    {
        json::json j_r;
        j_r["id"] = get_id(rhs);
        json::array preconditions;
        for (const auto &p : rhs.get_preconditions())
            preconditions.push_back(get_id(*p));
        j_r["preconditions"] = std::move(preconditions);
        j_r["effect"] = get_id(rhs.get_effect());
        j_r["rho"] = to_string(rhs.get_rho());
        j_r["state"] = rhs.get_solver().get_sat_core()->value(rhs.get_rho());
        j_r["intrinsic_cost"] = to_json(rhs.get_intrinsic_cost());
        j_r["data"] = rhs.get_data();
        return j_r;
    }
} // namespace ratio::solver
