#include "h_2.h"
#include "solver.h"
#include "enum_flaw.h"
#include "atom_flaw.h"
#include <cassert>

namespace ratio
{
    h_2::h_2(solver &s) : graph(s) {}

    void h_2::enqueue(flaw &f) { flaw_q.push_back(&f); }

    void h_2::propagate_costs(flaw &f)
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

    void h_2::build()
    {
        LOG("building the causal graph..");
        assert(s.get_sat_core().root_level());

        do
        {
            while (std::any_of(get_active_flaws().cbegin(), get_active_flaws().cend(), [](const auto &f)
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
                    {
                        expand_flaw(f);
#ifdef GRAPH_REFINING
                        if (auto e_f = dynamic_cast<enum_flaw *>(&f))
                            enum_flaws.push_back(e_f);
                        else if (auto a_f = dynamic_cast<atom_flaw *>(&f))
                            for (const auto &r : a_f->get_resolvers())
                                if (atom_flaw::is_unification(r.get()))
                                {
                                    auto &l = static_cast<atom_flaw &>(r.get().get_preconditions().front().get());
                                    if (s.get_sat_core().value(l.get_phi()) == utils::Undefined)
                                        landmarks.insert(&l);
                                }
#endif
                    }
                }
                flaw_q.pop_front();
            }

            // we check the graph for mutexes..
            check();

            // we extract the inconsistencies (and translate them into flaws)..
            get_incs();
#ifdef GRAPH_REFINING
            prune_enums();
#endif
        } while (std::any_of(get_active_flaws().cbegin(), get_active_flaws().cend(), [](const auto &f)
                             { return is_positive_infinite(f->get_estimated_cost()); }));

        // we perform some cleanings..
        if (!s.get_sat_core().simplify_db())
            throw riddle::unsolvable_exception();
    }

    void h_2::add_layer()
    {
        LOG("adding a layer to the causal graph..");
        assert(s.get_sat_core().root_level());
        assert(std::none_of(get_active_flaws().cbegin(), get_active_flaws().cend(), [](flaw *f)
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
                {
                    expand_flaw(f);
#ifdef GRAPH_REFINING
                    if (auto e_f = dynamic_cast<enum_flaw *>(&f))
                        enum_flaws.push_back(e_f);
                    else if (auto a_f = dynamic_cast<atom_flaw *>(&f))
                        for (const auto &r : a_f->get_resolvers())
                            if (atom_flaw::is_unification(r.get()))
                            {
                                auto &l = static_cast<atom_flaw &>(r.get().get_preconditions().front().get());
                                if (s.get_sat_core().value(l.get_phi()) == utils::Undefined)
                                    landmarks.insert(&l);
                            }
#endif
                }
                flaw_q.pop_front();
            }

            // we check the graph for mutexes..
            check();

            // we clear the enqueued flaws..
            c_res = nullptr;
            h_2_flaws.clear();

            // we extract the inconsistencies (and translate them into flaws)..
            get_incs();
#ifdef GRAPH_REFINING
            prune_enums();
#endif
        }

        // we check if we can further build the causal graph..
        if (std::any_of(get_active_flaws().cbegin(), get_active_flaws().cend(), [](const auto &f)
                        { return is_positive_infinite(f->get_estimated_cost()); }))
            build();
        else // we perform some cleanings..
            if (!s.get_sat_core().simplify_db())
                throw riddle::unsolvable_exception();
    }

#ifdef GRAPH_PRUNING
    void h_2::prune()
    {
        LOG("pruning the causal graph..");
        assert(s.get_sat_core().root_level());
        assert(std::none_of(get_active_flaws().cbegin(), get_active_flaws().cend(), [](flaw *f)
                            { return is_positive_infinite(f->get_estimated_cost()); }));

        for (const auto &f : flaw_q)
            if (already_closed.insert(f).second)
                if (!s.get_sat_core().new_clause({semitone::lit(get_gamma(), false), !f->get_phi()}))
                    throw riddle::unsolvable_exception();
        if (!s.get_sat_core().propagate())
            throw riddle::unsolvable_exception();
    }
#endif

#ifdef GRAPH_REFINING
    void h_2::refine()
    {
        LOG("checking landmarks..");
        // we sort the landmarks by decreasing estimated cost..
        std::vector<atom_flaw *> c_landmarks(landmarks.begin(), landmarks.end());
        std::sort(c_landmarks.begin(), c_landmarks.end(), [](const auto &l1, const auto &l2)
                  { return l1->get_estimated_cost() > l2->get_estimated_cost(); });
        for (auto l : c_landmarks)
        {
            if (s.get_sat_core().value(l->get_phi()) == utils::Undefined)
                s.get_sat_core().check({!l->get_phi()});
            if (s.get_sat_core().value(l->get_phi()) != utils::Undefined)
                landmarks.erase(l);
        }
    }

    void h_2::prune_enums()
    {
        LOG("checking enums..");
        // we sort the enums by decreasing estimated cost..
        std::sort(enum_flaws.begin(), enum_flaws.end(), [](const auto &e1, const auto &e2)
                  { return e1->get_estimated_cost() > e2->get_estimated_cost(); });
        for (auto e_f : enum_flaws)
            for (auto &r : e_f->get_resolvers())
                if (s.get_sat_core().value(r.get().get_rho()) == utils::Undefined)
                    s.get_sat_core().check({r.get().get_rho()});
    }
#endif

    void h_2::check()
    {
        bool sol = false;
        while (!sol)
        {
            sol = true;
            // we visit the flaws..
            for (auto &f : std::vector<flaw *>(get_active_flaws().begin(), get_active_flaws().end()))
                if (!visit(*f))
                    sol = false;

            if (pending_flaws.empty())
                break;
            else
            {
                for (auto &f : pending_flaws)
                    expand_flaw(*f);
                pending_flaws.clear();
            }
        }

        assert(s.get_sat_core().root_level());

        // we add the h_2 flaws..
        for (auto &f : h_2_flaws)
            new_flaw(f, false);

        // we clear the enqueued flaws..
        c_res = nullptr;
        h_2_flaws.clear();
    }

    bool h_2::visit(flaw &f)
    {
        LOG_DEBUG("visiting flaw " << to_string(f) << "..");
        bool solvable = true;

        // we visit the flaw..
        visited.insert(&f);

        std::vector<std::reference_wrapper<ratio::resolver>> c_resolvers(f.get_resolvers().begin(), f.get_resolvers().end());
        std::sort(c_resolvers.begin(), c_resolvers.end(), [](const auto &r1, const auto &r2)
                  { return r1.get().get_estimated_cost() < r2.get().get_estimated_cost(); });
        for (auto &r : c_resolvers)
            if (s.get_sat_core().value(r.get().get_rho()) != utils::False)
            {
                c_res = &r.get();
                if (!s.get_sat_core().assume(r.get().get_rho()))
                    throw riddle::unsolvable_exception();

                if (std::none_of(get_active_flaws().cbegin(), get_active_flaws().cend(), [](const auto &f)
                                 { return is_positive_infinite(f->get_estimated_cost()); }))
                {
                    if (s.get_sat_core().value(r.get().get_rho()) == utils::True) // we visit the subflaws..
                        for (auto &p : r.get().get_preconditions())
                            if (!visited.count(&p.get()) && !visit(p.get()))
                            {
                                solvable = false;
                                break;
                            }
                }
                else
                {
                    solvable = false;
                    for (auto &f : get_active_flaws())
                        if (is_positive_infinite(f->get_estimated_cost()) && !f->is_expanded())
                        {
                            flaw_q.erase(std::find(flaw_q.begin(), flaw_q.end(), f));
                            pending_flaws.insert(f);
                        }
                }

                if (s.get_sat_core().value(r.get().get_rho()) == utils::True)
                    s.get_sat_core().pop();

                if (solvable) // we have found a solution for the flaw..
                    break;
            }

        // we unvisit the flaw..
        visited.erase(&f);

        return solvable;
    }

    void h_2::negated_resolver(resolver &r)
    {
        // resolver c_res is mutex with r!
        assert(c_res != &r);

        if (c_res != nullptr &&                                                                                                // we are checking the graph..
            get_solver().get_idl_theory().distance(c_res->get_flaw().get_position(), r.get_flaw().get_position()).first > 0 && // the resolvers are not in the same causal chain..
            s.get_sat_core().value(c_res->get_rho()) == utils::True &&                                                         // the current resolver is active..
            s.get_sat_core().value(r.get_flaw().get_phi()) == utils::True &&                                                   // the negated resolver's flaw is active..
            (!mutexes.count(&r) || !mutexes.at(&r).count(c_res)))                                                              // the resolvers are not already mutex..
        {
            LOG("adding mutex between " << to_string(*c_res) << " and " << to_string(r) << "..");
            mutexes[&r].insert(c_res);
            mutexes[c_res].insert(&r);
        }

        // we refine the graph..
        graph::negated_resolver(r);
    }

    bool h_2::is_deferrable(flaw &f)
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

    h_2::h_2_flaw::h_2_flaw(flaw &sub_f, resolver &r, resolver &mtx_r) : flaw(sub_f.get_solver(), {r}), sub_f(sub_f), mtx_r(mtx_r) {}

    void h_2::h_2_flaw::compute_resolvers()
    {
        // we compute the resolvers..
        for (auto &r : sub_f.get_resolvers())
            if (&r.get() != &mtx_r)
                add_resolver(new h_2_resolver(*this, r.get()));
    }

    json::json h_2::h_2_flaw::get_data() const noexcept { return {{"type", "h_2"}, {"flaw", variable(sub_f.get_phi())}}; }

    h_2::h_2_flaw::h_2_resolver::h_2_resolver(h_2_flaw &f, resolver &sub_r) : resolver(f, sub_r.get_rho(), sub_r.get_intrinsic_cost()), sub_r(sub_r) {}

    void h_2::h_2_flaw::h_2_resolver::apply()
    {
        // we apply the resolver..
        for (auto &p : sub_r.get_preconditions())
            new_causal_link(p.get());
    }

    json::json h_2::h_2_flaw::h_2_resolver::get_data() const noexcept { return {{"type", "h_2"}, {"resolver", variable(sub_r.get_rho())}}; }
} // namespace ratio
