#include "h_1.h"
#include "solver.h"
#include <cassert>

namespace ratio
{
    h_1::h_1(solver &s) : graph(s) {}

    void h_1::enqueue(flaw &f) { flaw_q.push_back(&f); }

    void h_1::propagate_costs(flaw &f)
    {
        // the current flaw's cost..
        utils::rational c_cost = s.get_sat_core().value(f.get_phi()) == utils::False ? utils::rational::POSITIVE_INFINITY : f.get_best_resolver().get_estimated_cost();

        if (f.get_estimated_cost() == c_cost)
            return; // nothing to propagate..
        else if (visited.count(&f))
        { // we are propagating costs within a causal cycle..
            c_cost = utils::rational::POSITIVE_INFINITY;
            if (f.get_estimated_cost() == c_cost)
                return; // nothing to propagate..
        }

        // we update the cost of the flaw..
        set_cost(f, c_cost);

        // we (try to) update the estimated costs of the supports' effects and enqueue them for cost propagation..
        visited.insert(&f);
        for (const auto &supp : f.get_supports())
            if (s.get_sat_core().value(supp.get().get_rho()) != utils::False)
                propagate_costs(supp.get().get_flaw());
        visited.erase(&f);
    }

    void h_1::build()
    {
        LOG("building the causal graph..");
        assert(s.get_sat_core().root_level());

        while (std::any_of(get_flaws().cbegin(), get_flaws().cend(), [](const auto &f)
                           { return is_positive_infinite(f->get_estimated_cost()); }))
        {
            if (flaw_q.empty()) // we have no flaws to expand..
                throw riddle::unsolvable_exception();

            // we expand the flaw at the front of the queue..
            auto &f = *flaw_q.front();
            assert(!f.is_expanded());
            if (s.get_sat_core().value(f.get_phi()) != utils::False)
            {
#ifdef DEFERRABLE_FLAWS
                if (is_deferrable(f))
                    flaw_q.push_back(&f);
                else
#endif
                    expand_flaw(f);
            }
            flaw_q.pop_front();
        }

        // we extract the inconsistencies (and translate them into flaws)..
        get_incs();

        // we add the pending flaws to the planning graph..
        for (auto &f : flush_pending_flaws())
            new_flaw(std::move(f), false); // we add the flaws, without enqueuing, to the planning graph..

        // we perform some cleanings..
        if (!s.get_sat_core().simplify_db())
            throw riddle::unsolvable_exception();
    }

    void h_1::add_layer()
    {
        LOG("adding a layer to the causal graph..");
        assert(s.get_sat_core().root_level());
        assert(std::none_of(get_flaws().cbegin(), get_flaws().cend(), [](flaw *f)
                            { return is_positive_infinite(f->get_estimated_cost()); }));

        // we make a copy of the flaws queue..
        auto f_q = flaw_q;
        assert(std::all_of(f_q.cbegin(), f_q.cend(), [this](auto f)
                           { return is_deferrable(*f); }));
        while (std::all_of(f_q.cbegin(), f_q.cend(), [](auto f)
                           { return is_infinite(f->get_estimated_cost()); }))
        {
            if (flaw_q.empty()) // we have no flaws to expand..
                throw riddle::unsolvable_exception();
            // we expand all the flaws in the queue..
            auto q_size = flaw_q.size();
            for (size_t i = 0; i < q_size; ++i)
            {
                auto &f = *flaw_q.front();
                assert(!f.is_expanded());
                if (s.get_sat_core().value(f.get_phi()) != utils::False)
                    expand_flaw(f);
                flaw_q.pop_front();
            }
        }

        // we extract the inconsistencies (and translate them into flaws)..
        get_incs();

        // we add the pending flaws to the planning graph..
        for (auto &f : flush_pending_flaws())
            new_flaw(std::move(f), false); // we add the flaws, without enqueuing, to the planning graph..

        // we perform some cleanings..
        if (!s.get_sat_core().simplify_db())
            throw riddle::unsolvable_exception();
    }

#ifdef GRAPH_PRUNING
    void h_1::prune()
    {
        LOG("pruning the causal graph..");
        assert(s.get_sat_core().root_level());
        assert(std::none_of(get_flaws().cbegin(), get_flaws().cend(), [](flaw *f)
                            { return is_positive_infinite(f->get_estimated_cost()); }));

        for (const auto &f : flaw_q)
            if (already_closed.insert(f).second)
                if (!s.get_sat_core().new_clause({semitone::lit(get_gamma(), false), !f->get_phi()}))
                    throw riddle::unsolvable_exception();
        if (!s.get_sat_core().propagate())
            throw riddle::unsolvable_exception();
    }
#endif

    bool h_1::is_deferrable(flaw &f)
    {
        if (f.get_estimated_cost() < utils::rational::POSITIVE_INFINITY || std::any_of(f.get_resolvers().cbegin(), f.get_resolvers().cend(), [this](auto &r)
                                                                                       { return s.get_sat_core().value(r.get().get_rho()) == utils::True; }))
            return true; // we already have a possible solution for this flaw, thus we defer..
        if (s.get_sat_core().value(f.get_phi()) == utils::True || visited.count(&f))
            return false; // we necessarily have to solve this flaw: it cannot be deferred..
        visited.insert(&f);
        bool def = std::all_of(f.get_supports().cbegin(), f.get_supports().cend(), [this](auto &r)
                               { return is_deferrable(r.get().get_flaw()); });
        visited.erase(&f);
        return def;
    }
} // namespace ratio