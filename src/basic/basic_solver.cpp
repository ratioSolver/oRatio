#include "basic_solver.hpp"
#include "basic_flaws.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <stack>
#include <cassert>

namespace ratio
{
    basic_solver::basic_solver() noexcept : solver_core("oRatio Basic Solver")
    {
        read(INIT_STRING);
        // Initialize the root node..
        current_node = std::make_shared<Node>();
        fringe.push_back(current_node);
    }

    riddle::expr basic_solver::new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values)
    {
        assert(!values.empty());
        if (values.size() == 1)
        { // Single-valued enum
            assert(&values.front()->get_type() == &tp);
            return values.front();
        }
        else
        {
            std::vector<std::reference_wrapper<const utils::enum_val>> ev_refs;
            for (auto &ev_ptr : values)
                ev_refs.emplace_back(*ev_ptr);
            auto ev = ac_slv.new_var(ev_refs);
            // .. and create a new enum flaw to manage the variable..
            auto &ef = new_flaw<enum_flaw>(*this, get_causes(), std::make_shared<riddle::enum_item>(tp, std::move(values), ev));
            if (ef.get_causes().empty())
                current_node->open_flaws.insert(&ef);
            return ef.get_var();
        }
    }

    void basic_solver::new_clause(std::vector<riddle::bool_expr> &&exprs)
    {
        assert(!exprs.empty());
        if (exprs.size() == 1)
        { // if there is only one expression, just execute it..
            if (!execute(exprs[0]))
                throw std::runtime_error("Unsatisfiable constraints");
        }
        else
        { // otherwise, create a new clause flaw..
            std::vector<utils::lit> clause;
            clause.reserve(exprs.size());
            for (const riddle::bool_expr &expr : exprs)
                clause.push_back(static_cast<const riddle::bool_item &>(*expr).get_lit());

            add_constraint(ac_slv.new_clause(std::move(clause)));
            auto &cf = new_flaw<clause_flaw>(*this, get_causes(), std::move(exprs));
            if (cf.get_causes().empty())
                current_node->open_flaws.insert(&cf);
        }
    }
    void basic_solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        auto &df = new_flaw<disjunction_flaw>(*this, get_causes(), std::move(disjuncts));
        if (df.get_causes().empty())
            current_node->open_flaws.insert(&df);
    }

    void basic_solver::solve()
    {
        std::size_t node_id_counter = 0;
        while (!fringe.empty())
        {
            // Select the node with the least number of open flaws..
            auto min_it = std::min_element(fringe.begin(), fringe.end(),
                                           [](const std::shared_ptr<Node> &a, const std::shared_ptr<Node> &b)
                                           { return a->open_flaws.size() < b->open_flaws.size(); });
            LOG_DEBUG("Expanding node " << (*min_it)->id << " with " << (*min_it)->open_flaws.size() << " open flaws.");
            if (current_node != *min_it)
            { // Backtrack to the common ancestor..
                backtrack_to(find_common_ancestor(current_node, *min_it));
                // Move to the selected node..
                go_to(*min_it);
            }
            // Remove the selected node from the fringe..
            fringe.erase(min_it);
            if (!ac_slv.propagate() || !lin_slv.check())
                continue; // Conflict detected, backtrack..
            if (current_node->open_flaws.empty())
                return; // Solution found..
            // Select an open flaw to resolve..
            auto flaw_it = current_node->open_flaws.begin();
            auto &flw = **flaw_it;
            current_node->open_flaws.erase(flaw_it);
            LOG_DEBUG(flw.to_json().dump());
            // Compute the resolvers for the selected flaw..
            if (!flw.is_expanded())
            {
                compute_resolvers(flw);
                compute_flaw_cost(flw);
            }
            if (flw.get_resolvers().size() == 1)
            { // If there is only one resolver and applying it does not lead to a conflict, continue from the current node..
                auto &res = flw.get_resolvers().front().get();
                apply_resolver(res);
                if (ac_slv.propagate() && lin_slv.check())
                {
                    for (auto pre : res.get_preconditions())
                        current_node->open_flaws.insert(&pre.get());
                    fringe.push_back(current_node);
                }
            }
            else if (flw.get_resolvers().size() > 1) // Create a new child node for each resolver..
                for (auto &res : flw.get_resolvers())
                {
                    auto child_node = std::make_shared<Node>();
                    child_node->id = ++node_id_counter;
                    child_node->parent = current_node;
                    child_node->res = res;
                    child_node->open_flaws = current_node->open_flaws;
                    for (auto pre : res.get().get_preconditions())
                        child_node->open_flaws.insert(&pre.get());
                    fringe.push_back(child_node);
                }
            LOG_TRACE(to_json().dump());
        }
        throw std::runtime_error("No solution found");
    }

    riddle::atom_expr basic_solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args)
    {
        auto &af = new_flaw<atom_flaw>(*this, get_causes(), is_fact, pred, std::move(args), ac_slv.new_sat());
        if (af.get_causes().empty())
            current_node->open_flaws.insert(&af);
        return af.get_atom();
    }

    void basic_solver::compute_flaw_cost(flaw &f) noexcept
    {
        std::stack<std::pair<flaw *, std::unordered_set<flaw *>>> stk;
        stk.push({&f, {}}); // we push the flaw in the stack..

        while (!stk.empty())
        {
            auto c_f = stk.top();
            stk.pop();

            utils::rational c_cost = utils::rational::positive_infinite;
            for (const auto &res : c_f.first->get_resolvers())
                c_cost = std::min(c_cost, res.get().get_estimated_cost());

            if (c_f.first->get_estimated_cost() != c_cost) // we update the cost of the flaw..
            {
                set_flaw_cost(*c_f.first, c_cost);
                // we propagate the cost to the causes..
                for (auto &cause : c_f.first->get_causes())
                    stk.push({&cause.get().get_flaw(), c_f.second}); // we push the cause flaw in the stack..
                // we propagate the cost to the supported resolvers..
                for (auto &support : c_f.first->get_supports())
                    stk.push({&support.get().get_flaw(), c_f.second}); // we push the supported flaw in the stack..
            }
        }
    }

    std::shared_ptr<basic_solver::Node> basic_solver::find_common_ancestor(std::shared_ptr<Node> a, std::shared_ptr<Node> b) const
    {
        std::unordered_set<std::shared_ptr<Node>> ancestors;
        while (a)
        {
            ancestors.insert(a);
            a = a->parent;
        }
        while (b)
        {
            if (ancestors.count(b))
                return b;
            b = b->parent;
        }
        return nullptr;
    }

    void basic_solver::backtrack_to(const std::shared_ptr<Node> &lca)
    {
        while (current_node != lca)
        {
            retract_resolver(current_node->res->get());
            current_node = current_node->parent;
        }
    }

    void basic_solver::go_to(const std::shared_ptr<Node> &target)
    {
        std::vector<std::shared_ptr<Node>> path;
        auto temp_node = target;
        while (temp_node != current_node)
        {
            path.push_back(temp_node);
            temp_node = temp_node->parent;
        }
        for (auto it = path.rbegin(); it != path.rend(); ++it)
        {
            apply_resolver((*it)->res->get());
            current_node = *it;
        }
    }
} // namespace ratio
