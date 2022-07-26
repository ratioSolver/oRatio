#include "causal_graph.h"

namespace ratio::solver
{
    ORATIO_EXPORT causal_graph::causal_graph() {}

    ORATIO_EXPORT void causal_graph::init(solver &slv) { this->slv = &slv; }

    ORATIO_EXPORT void causal_graph::activated_flaw(flaw &) {}
    ORATIO_EXPORT void causal_graph::negated_flaw(flaw &f) { propagate_costs(f); }
    ORATIO_EXPORT void causal_graph::activated_resolver(resolver &) {}
    ORATIO_EXPORT void causal_graph::negated_resolver(resolver &r)
    {
        // if (slv.get_sat_core().value(r.get_effect().get_phi()) != False) // we update the cost of the resolver's effect..
        //     propagate_costs(r.get_effect());
    }
} // namespace ratio::solver
