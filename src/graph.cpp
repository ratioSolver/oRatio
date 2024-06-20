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

    bool graph::has_infinite_cost_active_flaws() const noexcept
    {
        return std::any_of(active_flaws.cbegin(), active_flaws.cend(), [](const auto &f)
                           { return is_infinite(f->get_estimated_cost()); });
    }

    flaw &graph::get_most_expensive_flaw() const noexcept
    {
        assert(!active_flaws.empty());
        return **std::max_element(active_flaws.cbegin(), active_flaws.cend(), [](const auto &lhs, const auto &rhs)
                                  { return lhs->get_estimated_cost() < rhs->get_estimated_cost(); });
    }

    void graph::build()
    {
        LOG_DEBUG("[" << slv.get_name() << "] Building the graph");
        assert(get_sat().root_level());

        do
        {
            while (has_infinite_cost_active_flaws())
            {
                if (flaw_q.empty())
                    throw riddle::unsolvable_exception();
                // we get the flaw to expand from the flaw queue..
                auto &f = flaw_q.front().get();
                if (get_sat().value(f.phi) != utils::False)
                    expand_flaw(f); // we expand the flaw..
                flaw_q.pop_front(); // we remove the flaw from the flaw queue..
            }
        } while (has_infinite_cost_active_flaws());
    }

    void graph::add_layer()
    {
        LOG_DEBUG("[" << slv.get_name() << "] Adding a new layer");
        assert(get_sat().root_level());
    }

    void graph::compute_flaw_cost(flaw &f) noexcept
    {
        // the current flaw's cost..
        auto c_cost = get_sat().value(f.get_phi()) == utils::False ? utils::rational::positive_infinite : f.get_best_resolver().get_estimated_cost();

        if (f.get_estimated_cost() == c_cost)
            return;                                 // the flaw's cost has not changed..
        else if (visited.find(&f) != visited.end()) // we are in a cycle..
        {
            if (f.get_estimated_cost() == utils::rational::positive_infinite)
                return; // nothing to propagate..
            else
                c_cost = utils::rational::positive_infinite;
        }

        if (!trail.empty()) // we store the current flaw's estimated cost, if not already stored, for allowing backtracking..
            trail.back().old_f_costs.emplace(&f, f.get_estimated_cost());
        f.est_cost = c_cost;      // we update the flaw's cost..
        FLAW_COST_CHANGED(f);     // we notify the flaw's cost has changed..
        FLAW_POSITION_CHANGED(f); // we notify the flaw's position has changed..

        visited.insert(&f); // we mark the flaw as visited..
        for (const auto &r : f.supports)
            if (get_sat().value(r.get().get_rho()) == utils::False)
                compute_flaw_cost(r.get().get_flaw()); // we propagate the cost change to the resolvers..
        visited.erase(&f);                             // we unmark the flaw..
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
            c_res = r;                 // we write down the resolver so that new flaws know their cause..
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
            restore_ni();         // we restore the controlling literal..
            c_res = std::nullopt; // we reset the resolver..
        }

        if (!get_sat().propagate())
            throw riddle::unsolvable_exception();

        if (get_sat().value(f.phi) == utils::True && std::none_of(f.resolvers.cbegin(), f.resolvers.cend(), [this](const auto &r)
                                                                  { return get_sat().value(r.get().rho) == utils::True; }))
            active_flaws.insert(&f); // the flaw is active and no resolver is active..

        // we compute the flaw's cost..
        compute_flaw_cost(f);
        assert(visited.empty());
    }

    bool graph::propagate(const utils::lit &p) noexcept
    {
        assert(cnfl.empty());
        assert(phis.count(variable(p)) || rhos.count(variable(p)));

        if (auto phi_it = phis.find(variable(p)); phi_it != phis.end()) // some flaws' state has changed..
            for (const auto &f : phi_it->second)
            {
                FLAW_STATE_CHANGED(*f); // we notify the flaw's state has changed..
                if (sign(p) == sign(f->get_phi()))
                { // the flaw is activated..
                    assert(get_sat().value(f->phi) == utils::True);
                    assert(!active_flaws.count(&*f));
                    if (!get_sat().root_level()) // we add the flaw to the new flaws..
                        trail.back().new_flaws.emplace(&*f);
                    if (std::none_of(f->resolvers.cbegin(), f->resolvers.cend(), [this](const auto &r)
                                     { return get_sat().value(r.get().get_rho()) == utils::True; })) // the flaw is active..
                        active_flaws.emplace(&*f);
                    else if (!get_sat().root_level()) // the flaw has been (accidentally) solved..
                        trail.back().solved_flaws.emplace(&*f);
                }
                else
                { // the flaw is negated..
                    assert(get_sat().value(f->phi) == utils::False);
                    assert(!active_flaws.count(&*f));
                    compute_flaw_cost(*f); // we compute the flaw's cost (i.e., infinite) and propagate it through the graph..
                }
            }

        if (auto rho_it = rhos.find(variable(p)); rho_it != rhos.end()) // some resolvers' state has changed..
            for (const auto &r : rho_it->second)
            {
                RESOLVER_STATE_CHANGED(*r); // we notify the resolver's state has changed..
                if (sign(p) == sign(r->get_rho()))
                { // the resolver is activated..
                    assert(get_sat().value(r->rho) == utils::True);
                    if (active_flaws.erase(&r->get_flaw()) && !get_sat().root_level()) // the flaw has been solved..
                        trail.back().solved_flaws.emplace(&r->get_flaw());
                }
                else // the resolver is negated..
                    assert(get_sat().value(r->rho) == utils::False);
            }

        return true;
    }

#ifdef ENABLE_VISUALIZATION
    json::json to_json(const graph &rhs) noexcept
    {
        json::json j;
        json::json flaws(json::json_type::array);
        for (const auto &f : rhs.get_flaws())
            flaws.push_back(to_json(f.get()));
        j["flaws"] = std::move(flaws);
        if (rhs.get_current_flaw())
            j["current_flaw"] = to_json(rhs.get_current_flaw().value().get());
        json::json resolvers(json::json_type::array);
        for (const auto &r : rhs.get_resolvers())
            resolvers.push_back(to_json(r.get()));
        j["resolvers"] = std::move(resolvers);
        if (rhs.get_current_resolver())
            j["current_resolver"] = to_json(rhs.get_current_resolver().value().get());
        return j;
    }
#endif
} // namespace ratio