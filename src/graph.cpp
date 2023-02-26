#include "graph.h"
#include "solver.h"
#include <cassert>

namespace ratio
{
    graph::graph(solver &s) : s(s), gamma(s.get_sat_core().new_var()) {}

    void graph::check()
    {
        assert(s.get_sat_core().root_level());
        switch (s.get_sat_core().value(gamma))
        {
        case utils::False:
            // we create a new gamma variable..
            gamma = s.get_sat_core().new_var();
            if (!s.get_active_flaws().empty())
            { // we check if we have an estimated solution for the current problem..
                if (std::any_of(s.get_active_flaws().cbegin(), s.get_active_flaws().cend(), [](const auto &f)
                                { return is_positive_infinite(f->get_estimated_cost()); }))
                    build(); // we build/extend the graph..
                else
                    add_layer(); // we add a layer to the current graph..
            }
            [[fallthrough]];
        case utils::Undefined:
#ifdef GRAPH_PRUNING
            prune(); // we prune the graph..
#endif
            // we take `gamma` decision..
            s.take_decision(semitone::lit(gamma));
        }
    }

    void graph::expand_flaw(flaw &f)
    {
        // we expand the flaw..
        s.expand_flaw(f);

        // we propagate the costs starting from the just expanded flaw..
        propagate_costs(f);
    }

    void graph::negated_resolver(resolver &r)
    {
        if (s.get_sat_core().value(r.get_flaw().get_phi()) != utils::False)
            // we propagate the costs starting from the just negated resolver..
            propagate_costs(r.get_flaw());
    }
} // namespace ratio
