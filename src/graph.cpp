#include "graph.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio
{
    graph::graph(solver &slv) noexcept : slv(slv) {}

    void graph::expand_flaw(flaw &f)
    {
        assert(!f.expanded);

        LOG_TRACE("[" << slv.get_name() << "] Expanding flaw");
        // we expand the flaw..
        f.expand();

        // we apply the flaw's resolvers..
        for (const auto &r : f.resolvers)
        {
            LOG_TRACE("[" << slv.get_name() << "] Applying resolver");
            res = r;
            try
            {
                r.get().apply();
            }
            catch (const std::exception &e)
            { // the resolver is inapplicable..
                if (!sat->new_clause({!r.get().get_rho()}))
                    throw riddle::unsolvable_exception();
            }
            res = std::nullopt;
        }

        switch (sat->value(f.get_phi()))
        {
        case utils::True: // we have a top-level (a landmark) flaw..
            if (std::none_of(f.get_resolvers().begin(), f.get_resolvers().end(), [this](const auto &r)
                             { return sat->value(r.get().get_rho()) == utils::True; }))
                active_flaws.insert(&f); // the flaw has not yet already been solved (e.g. it has a single resolver)..
            break;
        case utils::Undefined:           // we do not have a top-level (a landmark) flaw, nor an infeasible one..
            bind(variable(f.get_phi())); // we listen for the flaw to become active..
            break;
        }
    }
} // namespace ratio