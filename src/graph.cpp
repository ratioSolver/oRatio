#include "graph.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio
{
    graph::graph(solver &slv) noexcept : slv(slv) {}

    void graph::expand_flaw(flaw &f)
    {
        assert(!f.expanded);
        assert(sat->root_level());

        LOG_TRACE("[" << slv.get_name() << "] Expanding flaw");
        f.compute_resolvers(); // we compute the flaw's resolvers..
        f.expanded = true;     // we mark the flaw as expanded..

        if (f.resolvers.empty())
        { // there is no way for solving this flaw: we force the phi variable at false..
            if (!sat->new_clause({!f.get_phi()}))
                throw riddle::unsolvable_exception();
        }
        else
        { // we add causal relations between the flaw and its resolvers (i.e., if the flaw is phi exactly one of its resolvers should be in plan)..
            std::vector<utils::lit> rs;
            rs.reserve(f.resolvers.size() + 1);
            rs.push_back(!f.phi);
            for (const auto &r : f.resolvers)
                rs.push_back(r.get().get_rho());
            // activating the flaw results in one of its resolvers being activated..
            if (!sat->new_clause(std::move(rs)))
                throw riddle::unsolvable_exception();
            if (f.exclusive) // if the flaw is exclusive, we add mutual exclusion between its resolvers..
                for (size_t i = 0; i < f.resolvers.size(); ++i)
                    for (size_t j = i + 1; j < f.resolvers.size(); ++j)
                        if (!sat->new_clause({!f.resolvers[i].get().get_rho(), !f.resolvers[j].get().get_rho()}))
                            throw riddle::unsolvable_exception();

            // we apply the flaw's resolvers..
            for (const auto &r : f.resolvers)
            {
                LOG_TRACE("[" << slv.get_name() << "] Applying resolver");
                res = r;
                // activating the resolver results in the flaw being solved..
                if (!sat->new_clause({!r.get().get_rho(), f.get_phi()}))
                    throw riddle::unsolvable_exception();
                try
                { // we apply the resolver..
                    r.get().apply();
                }
                catch (const std::exception &e)
                { // the resolver is inapplicable..
                    if (!sat->new_clause({!r.get().get_rho()}))
                        throw riddle::unsolvable_exception();
                }
                res = std::nullopt;
            }
        }

        // we bind the variables to the SMT theory..
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
        for (const auto &r : f.resolvers)
            if (sat->value(r.get().get_rho()) == utils::Undefined)
                bind(variable(r.get().get_rho())); // we listen for the resolver to become active..
    }
} // namespace ratio