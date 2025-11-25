#include "basic_solver.hpp"
#include "basic_flaws.hpp"
#include "basic_types.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <stack>
#include <cassert>

#ifdef ORATIO_ENABLE_LISTENERS
#define STATE_CHANGED() state_changed()
#define NEW_NODE(n) new_node(n)
#else
#define STATE_CHANGED()
#define NEW_NODE(n)
#endif

namespace ratio
{
    node::node(std::shared_ptr<node> parent, std::optional<std::reference_wrapper<resolver>> res) noexcept : parent(std::move(parent)), res(res)
    {
        if (this->parent) // If there is a parent, inherit its open flaws..
            this->open_flaws = this->parent->open_flaws;
    }

    json::json node::to_json() const noexcept
    {
        json::json j{{"id", get_id()}};
        if (parent)
            j["parent"] = parent->get_id();
        if (res)
            j["resolver"] = res->get().to_json();
        json::json j_flaws(json::json_type::array);
        for (const auto &flw : open_flaws)
            j_flaws.push_back(flw->to_json());
        j["flaws"] = std::move(j_flaws);
        return j;
    }

    solver::solver() noexcept : solver_core("oRatio Basic Solver")
    {
        read(INIT_STRING);

        add_type(std::make_unique<basic_state_variable>(*this));
        add_type(std::make_unique<basic_reusable_resource>(*this));
        add_type(std::make_unique<basic_consumable_resource>(*this));

        // Initialize the root node..
        current_node = std::make_shared<node>();
        fringe.push_back(current_node);
    }

    riddle::expr solver::new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values)
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
            std::vector<std::reference_wrapper<resolver>> causes;
            if (current_node->res)
                causes.push_back(current_node->res->get());
            auto ef = std::make_shared<enum_flaw>(*this, std::move(causes), std::make_shared<riddle::enum_item>(tp, std::move(values), ev));
            current_node->open_flaws.insert(ef);
            return ef->get_var();
        }
    }

    void solver::new_clause(std::vector<riddle::bool_expr> &&exprs)
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

            auto &c = ac_slv.new_clause(std::move(clause));
            if (current_node->res)
                current_node->res->get().ctx.ac_cnsts.push_back(std::ref(c));
            else
                ac_slv.add_constraint(c);
            std::vector<std::reference_wrapper<resolver>> causes;
            if (current_node->res)
                causes.push_back(current_node->res->get());
            auto cf = std::make_shared<clause_flaw>(*this, std::move(causes), std::move(exprs));
            current_node->open_flaws.insert(cf);
        }
    }
    void solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<std::reference_wrapper<resolver>> causes;
        if (current_node->res)
            causes.push_back(current_node->res->get());
        auto df = std::make_shared<disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
        current_node->open_flaws.insert(df);
    }

    void solver::solve()
    {
        while (!fringe.empty())
        {
            // Select the node with the least number of open flaws..
            auto min_it = std::min_element(fringe.begin(), fringe.end(), [](const std::shared_ptr<node> &a, const std::shared_ptr<node> &b)
                                           { return a->open_flaws.size() < b->open_flaws.size(); });
            if (current_node != *min_it)
            { // Backtrack to the common ancestor..
                backtrack_to(find_common_ancestor(current_node, *min_it));
                // Move to the selected node..
                if (!go_to(*min_it))
                {
                    fringe.erase(min_it);
                    continue; // Conflict detected, backtrack..
                }
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
            flw.compute_resolvers();
            LOG_DEBUG("Flaw has " << flw.resolvers.size() << " resolvers.");
            switch (flw.resolvers.size())
            {
            case 0:
                continue; // No resolvers available, backtrack..
            case 1:
                if (current_node->res)
                    if (!apply_resolver(current_node->res->get()))
                        continue; // Conflict detected, backtrack..
                fringe.push_back(current_node);
                break;
            default:
                for (auto &res : flw.resolvers)
                {
                    auto n = std::make_shared<node>(current_node, *res);
                    NEW_NODE(*n);
                    fringe.push_back(n);
                }
                break;
            }
            STATE_CHANGED();
        }
        throw std::runtime_error("No solution found");
    }

    riddle::atom_expr solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args)
    {
        std::vector<std::reference_wrapper<resolver>> causes;
        if (current_node->res)
            causes.push_back(current_node->res->get());
        auto af = std::make_shared<atom_flaw>(*this, std::move(causes), is_fact, pred, std::move(args), ac_slv.new_sat());
        return af->get_atom();
    }

    std::shared_ptr<node> solver::find_common_ancestor(std::shared_ptr<node> a, std::shared_ptr<node> b) const
    {
        std::unordered_set<std::shared_ptr<node>> ancestors;
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

    void solver::backtrack_to(const std::shared_ptr<node> &lca) noexcept
    {
        while (current_node != lca)
        {
            lin_slv.retract(current_node->res->get().ctx.lin_cnsts);
            for (auto &ac_cnst : current_node->res->get().ctx.ac_cnsts)
                ac_slv.retract(ac_cnst.get());
            current_node = current_node->parent;
        }
    }

    bool solver::go_to(const std::shared_ptr<node> &target) noexcept
    {
        std::vector<std::shared_ptr<node>> path;
        auto temp_node = target;
        while (temp_node != current_node)
        {
            path.push_back(temp_node);
            temp_node = temp_node->parent;
        }
        for (auto it = path.rbegin(); it != path.rend(); ++it)
        {
            if (!apply_resolver((*it)->res->get()))
                return false;
            current_node = *it;
        }
        return true;
    }

    bool solver::apply_resolver(resolver &res) noexcept
    {
        if (!lin_slv.add_constraint(res.ctx.lin_cnsts))
            return false;
        if (!lin_slv.check())
        {
            lin_slv.retract(res.ctx.lin_cnsts);
            return false;
        }
        for (auto &ac_cnst : res.ctx.ac_cnsts)
            ac_slv.add_constraint(ac_cnst.get());

        if (!ac_slv.propagate())
        {
            lin_slv.retract(res.ctx.lin_cnsts);
            for (auto &ac_cnst : res.ctx.ac_cnsts)
                ac_slv.retract(ac_cnst.get());
            return false;
        }
        return true;
        return true;
    }
} // namespace ratio
