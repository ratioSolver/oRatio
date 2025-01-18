#include "graph.hpp"
#include "exceptions.hpp"
#include <algorithm>
#include <cassert>

namespace ratio
{
    graph::graph(std::string_view name) : core(name) {}

    json::json graph::to_json() const
    {
        json::json j_graph = core::to_json();
        return j_graph;
    }

    std::vector<std::reference_wrapper<flaw>> graph::get_flaws() const noexcept
    {
        std::vector<std::reference_wrapper<flaw>> fs;
        for (const auto &f : flaws)
            fs.push_back(*f);
        return fs;
    }

    std::vector<std::reference_wrapper<resolver>> graph::get_resolvers() const noexcept
    {
        std::vector<std::reference_wrapper<resolver>> rs;
        for (const auto &r : resolvers)
            rs.push_back(*r);
        return rs;
    }

    void graph::set_flaw_state(flaw &f, utils::lbool state) noexcept
    {
        if (f.state != state)
        {
            auto old_state = f.state;
            updating_flaw_state(f, old_state);
            f.state = state;
            FLAW_STATE_CHANGED(f);
            compute_flaw_cost(f);
        }
    }
    void graph::set_resolver_state(resolver &r, utils::lbool state) noexcept
    {
        if (r.state != state)
        {
            auto old_state = r.state;
            updating_resolver_state(r, old_state);
            r.state = state;
            RESOLVER_STATE_CHANGED(r);
            compute_flaw_cost(r.get_flaw());
        }
    }

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
        assert(!f.is_expanded()); // the flaw should not be expanded..
        set_current_flaw(f);      // set the current flaw..

        f.compute_resolvers(); // compute the resolvers for the current flaw..
        f.expanded = true;     // mark the flaw as expanded..
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
        utils::rational c_cost = utils::rational::positive_infinite;
        if (f.state == utils::False)
            for (const auto &res : f.resolvers)
                if (res.get().state != utils::False)
                    c_cost = std::min(c_cost, res.get().get_estimated_cost());

        if (f.est_cost != c_cost)
        { // we update the cost of the flaw..
            auto old_cost = f.est_cost;
            f.est_cost = c_cost;
            updating_flaw_cost(f, old_cost);
            FLAW_COST_CHANGED(f);

            // we propagate the cost to the supported resolvers..
            visited.insert(&f);
            for (auto &support : f.get_supports())
                compute_flaw_cost(support.get().f);
            visited.erase(&f);
        }
    }

    flaw::flaw(graph &gr, std::vector<std::reference_wrapper<resolver>> &&causes) : gr(gr), causes(causes)
    {
        for (auto &cause : causes)
        {
            cause.get().preconditions.push_back(*this); // this flaw is a precondition of its `cause` cause..
            supports.push_back(cause);                  // .. and it also supports the `cause` cause..
        }
    }

    json::json flaw::to_json() const
    {
        json::json j_flaw{{"cost", {{"num", static_cast<int64_t>(est_cost.numerator())}, {"den", static_cast<int64_t>(est_cost.denominator())}, {"state", to_string(get_state())}}}};
        json::json j_causes(json::json_type::array);
        for (const auto &c : causes)
            j_causes.push_back(static_cast<uint64_t>(c.get().get_id()));
        j_flaw["causes"] = std::move(j_causes);
        return j_flaw;
    }

    resolver::resolver(flaw &f, utils::rational &&intrinsic_cost) : f(f), intrinsic_cost(intrinsic_cost) { f.resolvers.push_back(*this); }

    utils::rational resolver::resolver::get_estimated_cost() const noexcept
    {
#ifdef H_ADD
        // we compute the cost of the resolver as the sum of its intrinsic cost and the estimated costs of its preconditions..
        return std::accumulate(preconditions.begin(), preconditions.end(), intrinsic_cost, [](const auto &lhs, const auto &prec)
                               { return lhs + prec.get().get_estimated_cost(); });
#endif
#ifdef H_MAX
        if (preconditions.empty())
            return intrinsic_cost;
        // we compute the cost of the resolver as the sum of its intrinsic cost and the maximum of its preconditions' estimated costs..
        return intrinsic_cost + std::max_element(preconditions.begin(), preconditions.end(), [](const auto &lhs, const auto &rhs)
                                                 { return lhs.get().get_estimated_cost() < rhs.get().get_estimated_cost(); })
                                    ->get()
                                    .get_estimated_cost();
#endif
    }

    json::json resolver::to_json() const
    {
        json::json j_resolver{{"cost", {{"num", static_cast<int64_t>(intrinsic_cost.numerator())}, {"den", static_cast<int64_t>(intrinsic_cost.denominator())}, {"state", to_string(get_state())}}}};
        json::json j_preconditions(json::json_type::array);
        for (const auto &p : preconditions)
            j_preconditions.push_back(static_cast<uint64_t>(p.get().get_id()));
        j_resolver["preconditions"] = std::move(j_preconditions);
        return j_resolver;
    }
} // namespace ratio