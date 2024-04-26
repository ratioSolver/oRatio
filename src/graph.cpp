#include "graph.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio
{
    graph::graph(solver &slv) noexcept : slv(slv) {}

    void graph::new_causal_link(flaw &f, resolver &r)
    {
        LOG_TRACE("[" << slv.get_name() << "] Creating causal link between flaw " << to_string(f.get_phi()) << " and resolver " << to_string(r.get_rho()));
        r.preconditions.push_back(f);
        f.supports.push_back(r);
        // activating the resolver requires the activation of the flaw..
        if (!slv.get_sat().new_clause({!r.get_rho(), f.get_phi()}))
            throw riddle::unsolvable_exception();
        // we introduce an ordering constraint..
        if (!slv.get_sat().new_clause({!r.get_rho(), slv.get_idl_theory().new_distance(r.get_flaw().position, f.position, 0)}))
            throw riddle::unsolvable_exception();
    }

    void graph::build()
    {
        LOG_DEBUG("[" << slv.get_name() << "] Building the graph");
        assert(get_sat().root_level());

        do
        {
            while (std::any_of(active_flaws.cbegin(), active_flaws.cend(), [](const auto &f)
                               { return is_positive_infinite(f->get_estimated_cost()); }))
            {
                if (flaw_q.empty())
                    throw riddle::unsolvable_exception();
                // we get the flaw to expand from the flaw queue..
                auto &f = flaw_q.front().get();
                if (get_sat().value(f.phi) != utils::False)
                    expand_flaw(f); // we expand the flaw..
                flaw_q.pop_front(); // we remove the flaw from the flaw queue..
            }
        } while (std::any_of(active_flaws.cbegin(), active_flaws.cend(), [](const auto &f)
                             { return is_positive_infinite(f->get_estimated_cost()); }));
    }

    void graph::add_layer()
    {
        LOG_DEBUG("[" << slv.get_name() << "] Adding a new layer");
        assert(get_sat().root_level());
    }

    void graph::expand_flaw(flaw &f)
    {
        assert(!f.expanded);
        assert(get_sat().root_level());

        LOG_TRACE("[" << slv.get_name() << "] Expanding flaw " << to_string(f.get_phi()));
        f.compute_resolvers(); // we compute the flaw's resolvers..
        f.expanded = true;     // we mark the flaw as expanded..

        // we add causal relations between the flaw and its resolvers (i.e., if the flaw is phi exactly one of its resolvers should be in plan)..
        std::vector<utils::lit> rs;
        rs.reserve(f.resolvers.size());
        for (const auto &r : f.resolvers)
            rs.push_back(r.get().get_rho());

        // activating the flaw results in one of its resolvers being activated..
        std::vector<utils::lit> lits = rs;
        lits.push_back(!f.phi);
        if (!get_sat().new_clause(std::move(lits)))
            throw riddle::unsolvable_exception();

        if (f.exclusive)
        { // we add mutual exclusion between the resolvers..
            for (size_t i = 0; i < rs.size(); ++i)
                for (size_t j = i + 1; j < rs.size(); ++j)
                    if (!get_sat().new_clause({!f.get_phi(), !rs[i], !rs[j]}))
                        throw riddle::unsolvable_exception();
            // if exactly one of the resolvers is activated, the flaw is solved..
            for (size_t i = 0; i < lits.size(); ++i)
            {
                lits = rs;
                lits[i] = !lits[i];
                lits.push_back(f.get_phi());
                if (!get_sat().new_clause(std::move(lits)))
                    throw riddle::unsolvable_exception();
            }
        }
        else // activating a resolver results in the flaw being solved..
            for (const auto &r : rs)
                if (!get_sat().new_clause({!r, f.get_phi()}))
                    throw riddle::unsolvable_exception();

        // we apply the flaw's resolvers..
        for (const auto &r : f.resolvers)
        {
            LOG_TRACE("[" << slv.get_name() << "] Applying resolver " << to_string(r.get().get_rho()));
            res = r;                   // we write down the resolver so that new flaws know their cause..
            set_ni(r.get().get_rho()); // we temporally set the resolver's rho as the controlling literal..

            // activating the resolver results in the flaw being solved..
            if (!get_sat().new_clause({!r.get().get_rho(), f.get_phi()}))
                throw riddle::unsolvable_exception();
            try
            { // we apply the resolver..
                r.get().apply();
            }
            catch (const std::exception &e)
            { // the resolver is inapplicable..
                if (!get_sat().new_clause({!r.get().get_rho()}))
                    throw riddle::unsolvable_exception();
            }
            restore_ni();       // we restore the controlling literal..
            res = std::nullopt; // we reset the resolver..
        }

        // we bind the variables to the SMT theory..
        switch (get_sat().value(f.get_phi()))
        {
        case utils::True: // we have a top-level (a landmark) flaw..
            if (std::none_of(f.get_resolvers().begin(), f.get_resolvers().end(), [this](const auto &r)
                             { return get_sat().value(r.get().get_rho()) == utils::True; }))
                active_flaws.insert(&f); // the flaw has not yet already been solved (e.g. it has a single resolver)..
            break;
        case utils::Undefined:           // we do not have a top-level (a landmark) flaw, nor an infeasible one..
            bind(variable(f.get_phi())); // we listen for the flaw to become active..
            break;
        }
        for (const auto &r : f.resolvers)
            if (get_sat().value(r.get().get_rho()) == utils::Undefined)
                bind(variable(r.get().get_rho())); // we listen for the resolver to become active..
    }
} // namespace ratio