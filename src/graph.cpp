#include "graph.hpp"
#include "exceptions.hpp"
#include <algorithm>
#include <cassert>

namespace ratio
{
    flaw::flaw(graph &gr, std::vector<std::reference_wrapper<resolver>> &&causes) : gr(gr), causes(causes)
    {
        for (auto &cause : causes)
            cause.get().preconditions.push_back(*this);
    }

    resolver::resolver(flaw &f, utils::rational &&intrinsic_cost) : f(f), intrinsic_cost(intrinsic_cost) { f.resolvers.push_back(*this); }

    utils::rational resolver::resolver::get_estimated_cost() const noexcept
    {
#ifdef H_ADD
        // we compute the cost of the resolver as the sum of its intrinsic cost and the estimated costs of its preconditions..
        return intrinsic_cost + std::accumulate(preconditions.begin(), preconditions.end(), utils::rational::zero, [](const auto &lhs, const auto &prec)
                                                { return lhs + prec.get().get_estimated_cost(); });
#endif
#ifdef H_MAX
        // we compute the cost of the resolver as the sum of its intrinsic cost and the maximum of its preconditions' estimated costs..
        return intrinsic_cost + std::max_element(preconditions.begin(), preconditions.end(), [](const auto &lhs, const auto &rhs)
                                                 { return lhs.get().get_estimated_cost() < rhs.get().get_estimated_cost(); })
                                    ->get()
                                    .get_estimated_cost();
#endif
    }

    graph::graph() {}

    std::vector<std::reference_wrapper<flaw>> graph::get_queued_flaws() const noexcept
    {
        std::vector<std::reference_wrapper<flaw>> res;
        for (const auto &flaw : flaw_q)
            res.push_back(flaw);
        return res;
    }

    void graph::build()
    {
        while (std::any_of(flaws.begin(), flaws.end(), [](const auto &flaw)
                           { return is_infinite(flaw->est_cost); }))
        { // while there are infinite cost flaws..
            if (flaw_q.empty())
                throw riddle::unsolvable_exception(); // if the flaw queue is empty, then the problem is unsolvable..

            auto &flaw = flaw_q.front().get();
            flaw_q.pop_front();
            expand_flaw(flaw);
        }
    }

    void graph::expand_flaws(const std::vector<std::reference_wrapper<flaw>> &flaws)
    {
        for (auto &flaw : flaws)
            expand_flaw(flaw.get());
    }

    void graph::expand_flaw(flaw &f)
    {
        set_current_flaw(f); // set the current flaw..

        f.compute_resolvers(); // compute the resolvers for the current flaw..
        expanded_flaw(f);      // notify the listeners that the flaw has been expanded (might be used for enforcing causality constraints)..

        for (auto &resolver : f.get_resolvers())
        {
            set_current_resolver(resolver); // set the current resolver..
            resolver.get().apply();         // we apply the resolver..
        }

        compute_flaw_cost(f);    // compute the cost of the flaw..
        assert(visited.empty()); // we should have visited all the flaws..

        set_current_resolver(std::nullopt); // reset the current resolver..
        set_current_flaw(std::nullopt);     // reset the current flaw..
    }

    void graph::compute_flaw_cost(flaw &f)
    {
        // we compute the cost of the flaw as the minimum cost of its resolvers..
        auto new_cost = visited.count(&f) ? utils::rational::positive_infinite : std::min_element(f.get_resolvers().begin(), f.get_resolvers().end(), [](const auto &lhs, const auto &rhs)
                                                                                                  { return lhs.get().get_estimated_cost() < rhs.get().get_estimated_cost(); })
                                                                                     ->get()
                                                                                     .get_estimated_cost();
        if (f.est_cost != new_cost)
        { // we update the cost of the flaw..
            auto old_cost = f.est_cost;
            f.est_cost = new_cost;
            flaw_cost_computed(f, old_cost);
            FLAW_COST_CHANGED(f);

            // we propagate the cost to the supported resolvers..
            visited.insert(&f);
            for (auto &support : f.get_supports())
                compute_flaw_cost(support.get().f);
            visited.erase(&f);
        }
    }
} // namespace ratio