#include "graph.hpp"
#include "exceptions.hpp"
#include "types.hpp"
#include "logging.hpp"
#include <algorithm>
#include <stack>
#include <cassert>

namespace ratio
{
    graph::graph(std::string_view name) : core(name) {}

    json::json graph::to_json() const
    {
        json::json j_graph = core::to_json();
        if (!flaws.empty())
        {
            json::json j_flaws;
            for (const auto &f : flaws)
                j_flaws[std::to_string(f->get_id())] = f->to_json();
            j_graph["flaws"] = std::move(j_flaws);
        }
        if (!resolvers.empty())
        {
            json::json j_resolvers;
            for (const auto &r : resolvers)
                j_resolvers[std::to_string(r->get_id())] = r->to_json();
            j_graph["resolvers"] = std::move(j_resolvers);
        }
        if (get_current_flaw().has_value())
            j_graph["current_flaw"] = static_cast<uint64_t>(get_current_flaw().value()->get_id());
        if (get_current_resolver().has_value())
            j_graph["current_resolver"] = static_cast<uint64_t>(get_current_resolver().value()->get_id());

        return j_graph;
    }

    std::vector<utils::ref_wrapper<flaw>> graph::get_flaws() const noexcept
    {
        std::vector<utils::ref_wrapper<flaw>> fs;
        for (const auto &f : flaws)
            fs.push_back(*f);
        return fs;
    }

    std::vector<utils::ref_wrapper<resolver>> graph::get_resolvers() const noexcept
    {
        std::vector<utils::ref_wrapper<resolver>> rs;
        for (const auto &r : resolvers)
            rs.push_back(*r);
        return rs;
    }

    void graph::set_flaw_state(flaw &f, utils::lbool state, bool resetting) noexcept
    {
        if (f.state != state)
        {
            f.state = state;
            FLAW_STATE_CHANGED(f);
            if (resetting) // if we are resetting, active flaws and cost estimates will be updated in the `pop` function..
                return;
            if (state == utils::True && std::none_of(f.get_resolvers().begin(), f.get_resolvers().end(), [](const auto &resolver)
                                                     { return resolver->state == utils::True; }))
            {
                if (!trail.empty()) // we store the current flaw as a new flaw, if not already stored, for allowing backtracking..
                    trail.back().new_flaws.emplace(&f);
                active_flaws.emplace(&f);
            }
            compute_flaw_cost(f);
        }
    }
    void graph::set_flaw_position(flaw &f, size_t pos) noexcept
    {
        if (f.position != pos)
        {
            f.position = pos;
            FLAW_POSITION_CHANGED(f);
        }
    }
    void graph::set_resolver_state(resolver &r, utils::lbool state, bool resetting) noexcept
    {
        if (r.state != state)
        {
            r.state = state;
            RESOLVER_STATE_CHANGED(r);
            if (resetting) // if we are resetting, active flaws and cost estimates will be updated in the `pop` function..
                return;
            if (state == utils::True)
            {
                if (!trail.empty()) // we store the resolver's flaw as a solved flaw, if not already stored, for allowing backtracking..
                    trail.back().solved_flaws.emplace(&r.get_flaw());
                active_flaws.erase(&r.get_flaw());
            }
            compute_flaw_cost(r.get_flaw());
        }
    }

    std::vector<utils::ref_wrapper<flaw>> graph::get_queued_flaws() const noexcept
    {
        std::vector<utils::ref_wrapper<flaw>> res;
        for (const auto &flaw : flaw_q)
            res.push_back(flaw);
        return res;
    }

    void graph::build()
    {
        LOG_DEBUG("[" << get_name() << "] Building the causal graph..");
        while (std::any_of(active_flaws.begin(), active_flaws.end(), [](const auto &f)
                           { return is_infinite(f->est_cost); }))
        { // while there are infinite cost flaws..
            if (flaw_q.empty())
                throw riddle::unsolvable_exception(); // if the flaw queue is empty, then the problem is unsolvable..

            auto &f = *flaw_q.front();
            set_current_flaw(f); // set the current flaw..
            flaw_q.pop_front();
            if (f.state != utils::False)
            {
                if (is_deferrable(f))
                    flaw_q.push_back(f);
                else
                    expand_flaw(f);
            }
            set_current_flaw(std::nullopt); // reset the current flaw..
        }
    }

    void graph::add_layer()
    {
        LOG_DEBUG("[" << get_name() << "] Expanding the causal graph..");
        assert(std::none_of(active_flaws.begin(), active_flaws.end(), [](const auto &f)
                            { return is_infinite(f->est_cost); })); // none of the active flaw should cost infinite (otherwise the build procedure should have been called)..

        if (flaw_q.empty())                       // we have no flaws to expand..
            throw riddle::unsolvable_exception(); // if the flaw queue is empty, then the problem is unsolvable..

        // we expand all the flaws in the queue..
        auto q_size = flaw_q.size();
        for (size_t i = 0; i < q_size; ++i)
        {
            auto &f = *flaw_q.front();
            set_current_flaw(f); // set the current flaw..
            flaw_q.pop_front();
            assert(!f.is_expanded());
            if (f.state != utils::False)
                expand_flaw(f);
            set_current_flaw(std::nullopt); // reset the current flaw..
        }
    }

    void graph::add_causal_link(flaw &f, resolver &r) noexcept
    {
        f.supports.push_back(r);
        r.preconditions.push_back(f);
        added_causal_link(f, r); // notify the listeners that a causal link has been added..
        NEW_CAUSAL_LINK(f, r);
    }

    void graph::expand_flaw(flaw &f)
    {
        assert(!f.is_expanded());        // the flaw should not be expanded..
        assert(f.state != utils::False); // the flaw should not be infeasible..

        f.compute_resolvers(); // compute the resolvers for the current flaw..
        f.expanded = true;     // mark the flaw as expanded..
        f.expanded_flaw();     // notify the listeners that the flaw has been expanded (might be used for enforcing causality constraints)..

        assert(std::none_of(f.get_resolvers().begin(), f.get_resolvers().end(), [](const auto &resolver)
                            { return resolver->state == utils::False; })); // all the resolvers should be feasible..
        for (auto &resolver : f.get_resolvers())
        {
            set_current_resolver(resolver); // set the current resolver..
            resolver->apply();              // we apply the resolver..
        }
        set_current_resolver(std::nullopt); // reset the current resolver..

        compute_flaw_cost(f); // compute the cost of the flaw..
    }

    void graph::compute_flaw_cost(flaw &f)
    {
        std::stack<std::pair<flaw *, std::unordered_set<flaw *>>> stk;
        stk.push({&f, {}}); // we push the flaw in the stack..

        while (!stk.empty())
        {
            auto c_f = stk.top();
            stk.pop();

            set_current_flaw(*c_f.first); // set the current flaw..
            utils::rational c_cost = utils::rational::positive_infinite;
            if (c_f.first->state != utils::False && c_f.second.insert(c_f.first).second) // we compute the cost of the flaw as the minimum of the costs of its resolvers..
                for (const auto &res : c_f.first->resolvers)
                {
                    set_current_resolver(*res); // set the current resolver..
                    if (res->state != utils::False)
                        c_cost = std::min(c_cost, res->get_estimated_cost());
                    set_current_resolver(std::nullopt); // reset the current resolver..
                }

            if (c_f.first->est_cost != c_cost) // we update the cost of the flaw..
            {
                if (!trail.empty()) // we store the current flaw's estimated cost, if not already stored, for allowing backtracking..
                    trail.back().old_f_costs.emplace(c_f.first, c_f.first->est_cost);

                c_f.first->est_cost = c_cost;
                FLAW_COST_CHANGED(*c_f.first);

                // we propagate the cost to the supported resolvers..
                for (auto &support : c_f.first->get_supports())
                    stk.push({&support->f, c_f.second}); // we push the supported flaw in the stack..
            }
            set_current_flaw(std::nullopt); // reset the current flaw..
        }
    }

    void graph::push() noexcept
    {
        LOG_DEBUG("[" << get_name() << "] " << std::to_string(trail.size()) << " (" << std::to_string(active_flaws.size()) << ")");
        trail.push_back({}); // we push a new trail..
    }

    void graph::pop() noexcept
    {
        assert(!trail.empty());
        auto &t = trail.back();
        // we restore the previous state of the graph..
        for (const auto &f : t.solved_flaws)
            if (f->state == utils::True && std::none_of(f->get_resolvers().cbegin(), f->get_resolvers().cend(), [](const auto &r)
                                                        { return r->state == utils::True; }))
                active_flaws.emplace(f); // we restore the solved flaws..
        for (const auto &f : t.new_flaws)
            active_flaws.erase(f); // we remove the new flaws..
        assert(std::all_of(active_flaws.begin(), active_flaws.end(), [](const auto &f)
                           { return f->state == utils::True; })); // all the active flaws should be active..
        for (const auto &[f, c] : t.old_f_costs)
        { // we restore the flaws' costs..
            assert(f->est_cost != c);
            f->est_cost = c;
            FLAW_COST_CHANGED(*f);
        }
        trail.pop_back();
    }

    bool graph::is_deferrable(flaw &f)
    {
        std::unordered_set<flaw *> visited;
        std::stack<flaw *> stk;
        stk.push(&f);

        while (!stk.empty())
        {
            flaw *c_f = stk.top();
            stk.pop();

            if (c_f->get_estimated_cost() < utils::rational::positive_infinite || std::any_of(c_f->get_resolvers().cbegin(), c_f->get_resolvers().cend(), [this](auto &r)
                                                                                              { return r->state == utils::True; }))
                continue; // deferrable, check nothing more..

            if (c_f->state == utils::True || !visited.insert(c_f).second)
                return false; // not deferrable..

            for (const auto &support : c_f->get_supports())
                stk.push(&support->get_flaw()); // schedule to be checked..
        }
        // if stack empties, all reachable flaws were deferrable
        return true;
    }

    flaw::flaw(graph &gr, std::vector<utils::ref_wrapper<resolver>> &&causes, const bool &exclusive) : gr(gr), causes(causes), exclusive(exclusive)
    {
        for (auto &cause : causes)
        {
            cause->preconditions.push_back(*this); // this flaw is a precondition of its `cause` cause..
            supports.push_back(cause);             // .. and it also supports the `cause` cause..
        }
    }

    void flaw::set_state(utils::lbool state) noexcept
    {
        assert(gr.trail.empty());
        if (this->state != state)
        {
            this->state = state;
            if (state == utils::True && std::none_of(resolvers.begin(), resolvers.end(), [](const auto &resolver)
                                                     { return resolver->state == utils::True; }))
                gr.active_flaws.emplace(this);
        }
    }

    json::json flaw::to_json() const
    {
        json::json j_flaw{{"cost", {{"num", static_cast<int64_t>(est_cost.numerator())}, {"den", static_cast<int64_t>(est_cost.denominator())}}}, {"state", to_string(state)}, {"position", static_cast<uint64_t>(position)}};
        if (!causes.empty())
        {
            json::json j_causes(json::json_type::array);
            for (const auto &c : causes)
                j_causes.push_back(static_cast<uint64_t>(c->get_id()));
            j_flaw["causes"] = std::move(j_causes);
        }
        if (!supports.empty())
        {
            json::json j_supports(json::json_type::array);
            for (const auto &s : supports)
                j_supports.push_back(static_cast<uint64_t>(s->get_id()));
            j_flaw["supports"] = std::move(j_supports);
        }
        return j_flaw;
    }

    resolver::resolver(flaw &f, utils::rational &&intrinsic_cost) : f(f), intrinsic_cost(intrinsic_cost) { f.resolvers.push_back(*this); }

    utils::rational resolver::resolver::get_estimated_cost() const noexcept
    {
        if (state == utils::False)
            return utils::rational::positive_infinite;
        else if (preconditions.empty())
            return intrinsic_cost;
#ifdef H_ADD
        // we compute the cost of the resolver as the sum of its intrinsic cost and the estimated costs of its preconditions..
        return std::accumulate(preconditions.begin(), preconditions.end(), intrinsic_cost, [](const auto &lhs, const auto &prec)
                               { return lhs + prec->get_estimated_cost(); });
#endif
#ifdef H_MAX
        // we compute the cost of the resolver as the sum of its intrinsic cost and the maximum of its preconditions' estimated costs..
        return intrinsic_cost + (*std::max_element(preconditions.begin(), preconditions.end(), [](const auto &lhs, const auto &rhs)
                                                   { return lhs->get_estimated_cost() < rhs->get_estimated_cost(); }))
                                    ->get_estimated_cost();
#endif
    }

    void resolver::set_state(utils::lbool state) noexcept
    {
        assert(get_flaw().gr.trail.empty());
        if (this->state != state)
        {
            this->state = state;
            if (state == utils::True)
                get_flaw().gr.active_flaws.erase(&f);
        }
    }

    json::json resolver::to_json() const
    {
        json::json j_resolver{{"flaw", static_cast<uint64_t>(f.get_id())}, {"intrinsic_cost", {{"num", static_cast<int64_t>(intrinsic_cost.numerator())}, {"den", static_cast<int64_t>(intrinsic_cost.denominator())}}}, {"state", to_string(state)}};
        if (!preconditions.empty())
        {
            json::json j_preconditions(json::json_type::array);
            for (const auto &p : preconditions)
                j_preconditions.push_back(static_cast<uint64_t>(p->get_id()));
            j_resolver["preconditions"] = std::move(j_preconditions);
        }
        return j_resolver;
    }
} // namespace ratio