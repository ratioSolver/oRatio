#include "graph.hpp"
#include "smart_type.hpp"
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

    void graph::solve_inconsistencies()
    {
        LOG_DEBUG("[" << slv.get_name() << "] Solving inconsistencies");

        auto incs = get_incs(); // all the current inconsistencies..
        LOG_DEBUG("[" << slv.get_name() << "] Found " << incs.size() << " inconsistencies");

        // we solve the inconsistencies..
        while (!incs.empty())
        {
            if (const auto &uns_inc = std::find_if(incs.cbegin(), incs.cend(), [](const auto &v)
                                                   { return v.empty(); });
                uns_inc != incs.cend())
            { // we have an unsolvable inconsistency..
                LOG_DEBUG("[" << slv.get_name() << "] Unsatisfiable inconsistency");
                slv.next(); // we move to the next state..
            }
            else if (const auto &inc = std::find_if(incs.cbegin(), incs.cend(), [](const auto &v)
                                                    { return v.size() == 1; });
                     inc != incs.cend())
            { // we have a trivial inconsistency..
                LOG_DEBUG("[" << slv.get_name() << "] Trivial inconsistency");
                assert(get_sat().value(inc->front().first) == utils::Undefined);
                // we can learn something from it..
                std::vector<utils::lit> learnt;
                learnt.push_back(inc->front().first);
                for (const auto &l : get_sat().get_decisions())
                    learnt.push_back(!l);
                record(std::move(learnt));
                if (!get_sat().propagate())
                    throw riddle::unsolvable_exception();
            }
            else
            { // we have a non-trivial inconsistency, so we have to take a decision..
                std::vector<std::pair<utils::lit, double>> bst_inc;
                double k_inv = std::numeric_limits<double>::infinity();
                for (const auto &inc : incs)
                {
                    double bst_commit = std::numeric_limits<double>::infinity();
                    for ([[maybe_unused]] const auto &[choice, commit] : inc)
                        if (commit < bst_commit)
                            bst_commit = commit;
                    double c_k_inv = 0;
                    for ([[maybe_unused]] const auto &[choice, commit] : inc)
                        c_k_inv += 1l / (1l + (commit - bst_commit));
                    if (c_k_inv < k_inv)
                    {
                        k_inv = c_k_inv;
                        bst_inc = inc;
                    }
                }

                // we select the best choice (i.e. the least committing one) from those available for the best flaw..
                slv.take_decision(std::min_element(bst_inc.cbegin(), bst_inc.cend(), [](const auto &ch0, const auto &ch1)
                                                   { return ch0.second < ch1.second; })
                                      ->first);
            }

            incs = get_incs(); // we get the new inconsistencies..
            LOG_DEBUG("[" << slv.get_name() << "] Found " << incs.size() << " inconsistencies");
        }

        LOG_DEBUG("[" << slv.get_name() << "] Inconsistencies solved");
    }

    std::vector<std::vector<std::pair<utils::lit, double>>> graph::get_incs() const noexcept
    {
        std::vector<std::vector<std::pair<utils::lit, double>>> incs;
        for (const auto &st : smart_types)
        {
            auto st_incs = st->get_current_incs();
            incs.insert(incs.end(), st_incs.begin(), st_incs.end());
        }
        return incs;
    }

    void graph::reset_smart_types() noexcept
    {
        // we reset the smart types..
        smart_types.clear();
        // we seek for the existing smart types..
        std::queue<riddle::component_type *> q;
        for (const auto &t : slv.get_types())
            if (auto ct = dynamic_cast<riddle::component_type *>(&t.get()))
                q.push(ct);
        while (!q.empty())
        {
            auto ct = q.front();
            q.pop();
            if (const auto st = dynamic_cast<smart_type *>(ct); st)
                smart_types.emplace_back(st);
            for (const auto &t : ct->get_types())
                if (auto c_ct = dynamic_cast<riddle::component_type *>(&t.get()))
                    q.push(c_ct);
        }
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