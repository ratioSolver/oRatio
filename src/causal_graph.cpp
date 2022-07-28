#include "causal_graph.h"
#include "solver.h"
#include "flaw.h"
#include "resolver.h"
#include <algorithm>
#include <cassert>

namespace ratio::solver
{
    ORATIO_EXPORT causal_graph::causal_graph() {}

    ORATIO_EXPORT void causal_graph::init(solver &slv) { this->slv = &slv; }

    ORATIO_EXPORT void causal_graph::activated_flaw(flaw &) {}
    ORATIO_EXPORT void causal_graph::negated_flaw(flaw &f) { propagate_costs(f); }
    ORATIO_EXPORT void causal_graph::activated_resolver(resolver &) {}
    ORATIO_EXPORT void causal_graph::negated_resolver(resolver &r)
    {
        if (slv->get_sat()->value(r.get_effect().get_phi()) != semitone::False)
            // we update the cost of the resolver's effect..
            propagate_costs(r.get_effect());
    }

    void causal_graph::check()
    {
        assert(slv->root_level());
        switch (slv->get_sat()->value(gamma))
        {
        case semitone::False:
            init(*slv); // we create a new graph var..
            if (!slv->get_active_flaws().empty())
            { // we check if we have an estimated solution for the current problem..
                if (std::any_of(slv->get_active_flaws().cbegin(), slv->get_active_flaws().cend(), [](flaw *f)
                                { return is_positive_infinite(f->get_estimated_cost()); }))
                    build(); // we build/extend the graph..
                else
                    add_layer(); // we add a layer to the current graph..
            }
            [[fallthrough]];
        case semitone::Undefined:
#ifdef GRAPH_PRUNING
            prune(); // we prune the graph..
#endif
            // we take 'gamma' decision..
            slv->take_decision(semitone::lit(gamma));
        }
    }

    void causal_graph::expand_flaw(flaw &f)
    {
        // we expand the flaw..
        slv->expand_flaw(f);

        // we propagate the costs starting from the just expanded flaw..
        propagate_costs(f);
    }
} // namespace ratio::solver
