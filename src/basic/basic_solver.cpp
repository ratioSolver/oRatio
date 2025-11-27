#include "basic_solver.hpp"
#include "basic_flaws.hpp"
#include "basic_types.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <stack>
#include <cassert>

#ifdef ORATIO_ENABLE_LISTENERS
#define STATE_CHANGED() state_changed()
#define NEW_NODE(n) node_created(n)
#define FLAW_CREATED(n, f) flaw_created(n, f)
#define RESOLVER_APPLIED(n, r) resolver_applied(n, r)
#define INCONSISTENT_NODE(n) inconsistent_node(n)
#define CURRENT_NODE(n) current_node(n)
#else
#define STATE_CHANGED()
#define NEW_NODE(n)
#define FLAW_CREATED(n, f)
#define RESOLVER_APPLIED(n, r)
#define INCONSISTENT_NODE(n)
#define CURRENT_NODE(n)
#endif

namespace ratio
{
    riddle::expr enum_item::get(std::string_view name)
    {
        assert(get_values().size() > 1); // should not be a singleton..

        if (auto it = items.find(name.data()); it != items.end())
            return it->second;

        // different referenced values can represent the same item, so we group them by the item they represent..
        std::unordered_set<riddle::expr> matching_values;
        for (const auto &v : get_values())
            matching_values.emplace(std::dynamic_pointer_cast<riddle::env>(v)->get(name));
        assert(!matching_values.empty());

        if (matching_values.size() == 1)
        { // we are lucky!
            items.emplace(name, *matching_values.begin());
            return *matching_values.begin();
        }
        // we have to create a new variable :(

        auto &tp = static_cast<riddle::component_type &>(get_type()).get_field(name).get_type(); // the target type..

        if (flw.get_resolvers().empty())
            flw.compute_resolvers();

        if (is_bool(tp))
        { // we create a new boolean item..
            auto b = std::dynamic_pointer_cast<riddle::bool_item>(get_core().new_bool());
            // we force the variable to assume the same value of the referenced bools according to the value of the enum..
            for (auto &res : flw.get_resolvers())
            {
                auto &er = static_cast<choose_val &>(*res);
                auto &erv = static_cast<riddle::bool_item &>(*std::dynamic_pointer_cast<riddle::env>(er.get_value())->get(name));
                if (utils::sign(erv.get_lit()) == utils::sign(b->get_lit()))
                    er.ctx.ac_cnsts.push_back(flw.get_ac().new_equal(utils::variable(erv.get_lit()), utils::variable(b->get_lit())));
                else
                    er.ctx.ac_cnsts.push_back(flw.get_ac().new_distinct(utils::variable(erv.get_lit()), utils::variable(b->get_lit())));
            }
            items.emplace(name, b);
            return b;
        }
        else if (is_int(tp) || is_real(tp))
        {
            auto min = utils::inf_rational(utils::rational::positive_infinite);
            auto max = utils::inf_rational(utils::rational::negative_infinite);
            for (const auto &val : matching_values)
            {
                const auto &a_itm = static_cast<riddle::arith_item &>(*val);
                const auto c_min = static_cast<solver &>(get_core()).lin_slv.lb(a_itm.get_lin());
                if (min < c_min)
                    min = c_min;
                const auto c_max = static_cast<solver &>(get_core()).lin_slv.ub(a_itm.get_lin());
                if (max > c_max)
                    max = c_max;
            }
            if (min == max)
            { // we are lucky! we have a constant..
                if (is_int(tp))
                {
                    assert(min.get_infinitesimal() == 0);
                    assert(min.get_rational().denominator() == 1);
                    auto i = get_core().new_int(min.get_rational().numerator());
                    items.emplace(name, i);
                    return i;
                }
                else
                {
                    assert(is_real(tp));
                    assert(min.get_infinitesimal() == 0);
                    auto i = get_core().new_real(min.get_rational());
                    items.emplace(name, i);
                    return i;
                }
            }
            else
            { // we need to create a new variable..
                auto ai = std::dynamic_pointer_cast<riddle::arith_item>(is_int(tp) ? get_core().new_int() : get_core().new_real());
                // we force the variable to assume the same value of the referenced ariths according to the value of the enum..
                for (auto &res : flw.get_resolvers())
                {
                    auto &er = static_cast<choose_val &>(*res);
                    auto &erv = static_cast<riddle::arith_item &>(*std::dynamic_pointer_cast<riddle::env>(er.get_value())->get(name));
                    [[maybe_unused]] bool valid = flw.get_lin().new_eq(erv.get_lin(), ai->get_lin(), er.ctx.lin_cnsts);
                    assert(valid);
                    flw.get_lin().retract(er.ctx.lin_cnsts);
                }
                items.emplace(name, ai);
                return ai;
            }
        }
        else
        {
            std::vector<riddle::expr> vals;
            for (const auto &val : matching_values)
                vals.push_back(val);
            auto e = get_core().new_enum(static_cast<riddle::component_type &>(tp), std::move(vals));
            for (auto &res : flw.get_resolvers())
            {
                auto &er = static_cast<choose_val &>(*res);
                auto &erv = static_cast<utils::enum_val &>(*std::dynamic_pointer_cast<riddle::env>(er.get_value())->get(name));
                er.ctx.ac_cnsts.push_back(flw.get_ac().new_assign(static_cast<const riddle::enum_item &>(*e).get_var(), erv));
            }
            items.emplace(name, e);
            return e;
        }
    }

    node::node(std::optional<std::reference_wrapper<node>> parent) noexcept : parent(std::move(parent))
    {
        if (this->parent) // If there is a parent, inherit its open flaws..
            this->open_flaws = this->parent->get().open_flaws;
    }

    double node::get_estimated_cost() const noexcept
    {
        std::size_t depth = 0;
        for (auto p = parent; p; p = p->get().parent)
            ++depth;
        return depth + open_flaws.size();
    }

    json::json node::to_json() const noexcept
    {
        json::json j{{"id", get_id()}};
        if (parent)
            j["parent"] = parent->get().get_id();
        j["consistent"] = consistent;
        json::json j_ress;
        for (const auto &res : resolvers)
            j_ress[res->get_id()] = res->to_json();
        j["resolvers"] = std::move(j_ress);
        json::json j_flaws;
        for (const auto &flw : open_flaws)
            j_flaws[std::to_string(flw->get_id())] = flw->to_json();
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
        auto root = std::make_unique<node>();
        NEW_NODE(*root);
        c_node = *root;
        fringe.push_back(*root);
        nodes.push_back(std::move(root));
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
            if (!c_node->get().resolvers.empty())
                causes.push_back(*c_node->get().resolvers.back());
            auto &ef = new_flaw<enum_flaw>(*this, std::move(causes), tp, std::move(values), ev);
            return ef.get_var();
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
            if (!c_node->get().resolvers.empty())
                c_node->get().resolvers.back()->ctx.ac_cnsts.push_back(std::ref(c));
            else
                ac_slv.add_constraint(c);
            std::vector<std::reference_wrapper<resolver>> causes;
            if (!c_node->get().resolvers.empty())
                causes.push_back(*c_node->get().resolvers.back());
            new_flaw<clause_flaw>(*this, std::move(causes), std::move(exprs));
        }
    }
    void solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<std::reference_wrapper<resolver>> causes;
        if (!c_node->get().resolvers.empty())
            causes.push_back(*c_node->get().resolvers.back());
        new_flaw<disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
    }

    void solver::solve()
    {
        while (!fringe.empty())
        {
            // Select the node with the least estimated cost..
            auto min_it = std::min_element(fringe.begin(), fringe.end(), [](const auto &a, const auto &b)
                                           { return a.get().get_estimated_cost() < b.get().get_estimated_cost(); });
            if (&c_node->get() != &min_it->get())
            { // Backtrack to the common ancestor..
                backtrack_to(find_common_ancestor(c_node->get(), min_it->get()));
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
            if (c_node->get().open_flaws.empty())
                return; // Solution found..
            // Select the open flaw with the least estimated cost..
            auto flw_it = std::min_element(c_node->get().open_flaws.begin(), c_node->get().open_flaws.end(), [](const auto &a, const auto &b)
                                           { return a->get_estimated_cost() < b->get_estimated_cost(); });
            auto c_flw = *flw_it;
            // Move the selected flaw to the closed flaws..
            c_node->get().open_flaws.erase(c_flw);
            c_node->get().closed_flaws.insert(c_flw);
            LOG_DEBUG(c_flw->to_json().dump());
            // Compute the resolvers for the selected flaw..
            c_flw->compute_resolvers();
            LOG_DEBUG("Flaw has " << c_flw->resolvers.size() << " resolvers.");
            switch (c_flw->resolvers.size())
            {
            case 0: // No resolvers available, backtrack..
                continue;
            case 1: // Only one resolver, apply it directly..
                if (c_flw->resolvers.at(0)->apply())
                {
                    RESOLVER_APPLIED(c_node->get(), *c_flw->resolvers.at(0));
                    c_node->get().resolvers.push_back(c_flw->resolvers.at(0));
                    fringe.push_back(c_node->get());
                }
                else
                {
                    INCONSISTENT_NODE(c_node->get());
                    lin_slv.retract(c_flw->resolvers.at(0)->ctx.lin_cnsts);
                    for (auto &ac_cnst : c_flw->resolvers.at(0)->ctx.ac_cnsts)
                        ac_slv.retract(ac_cnst.get());
                }
                break;
            default: // Multiple resolvers, create a new node for each..
                for (auto &res : c_flw->resolvers)
                {
                    auto n = std::make_unique<node>(c_node->get());
                    NEW_NODE(*n);
                    c_node = *n;
                    CURRENT_NODE(*n);
                    bool apply = res->apply();
                    lin_slv.retract(res->ctx.lin_cnsts);
                    for (auto &ac_cnst : res->ctx.ac_cnsts)
                        ac_slv.retract(ac_cnst.get());
                    c_node = n->parent;
                    CURRENT_NODE(c_node);
                    if (apply)
                    {
                        n->resolvers.push_back(res);
                        fringe.push_back(*n);
                        nodes.push_back(std::move(n));
                    }
                }
                break;
            }
            c_flw->resolvers.clear();
            STATE_CHANGED();
        }
        throw std::runtime_error("No solution found");
    }

    json::json solver::to_json() const
    {
        json::json j = core::to_json();
        json::json j_nodes;
        for (const auto &n : nodes)
            j_nodes[std::to_string(n->get_id())] = n->to_json();
        j["nodes"] = std::move(j_nodes);
        return j;
    }

    riddle::atom_expr solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args)
    {
        std::vector<std::reference_wrapper<resolver>> causes;
        if (!c_node->get().resolvers.empty())
            causes.push_back(*c_node->get().resolvers.back());
        auto &af = new_flaw<atom_flaw>(*this, std::move(causes), is_fact, pred, std::move(args), ac_slv.new_sat());
        return af.get_atom();
    }

    const node &solver::find_common_ancestor(const node &a, const node &b) const
    {
        std::unordered_set<const node *> ancestors;
        auto c_a = &a;
        while (c_a)
        {
            ancestors.insert(c_a);
            c_a = c_a->parent ? &c_a->parent->get() : nullptr;
        }
        auto c_b = &b;
        while (c_b)
        {
            if (ancestors.count(c_b))
                return *c_b;
            c_b = c_b->parent ? &c_b->parent->get() : nullptr;
        }
        throw std::runtime_error("No common ancestor found");
    }

    void solver::backtrack_to(const node &lca) noexcept
    {
        while (&c_node->get() != &lca)
        {
            for (auto &res : c_node->get().resolvers)
            {
                lin_slv.retract(res->ctx.lin_cnsts);
                for (auto &ac_cnst : res->ctx.ac_cnsts)
                    ac_slv.retract(ac_cnst.get());
            }
            c_node = *c_node->get().parent;
            CURRENT_NODE(c_node);
        }
    }

    bool solver::go_to(const node &target) noexcept
    {
        std::vector<const node *> path;
        auto temp_node = &target;
        while (temp_node != &c_node->get())
        {
            path.push_back(temp_node);
            temp_node = &temp_node->parent->get();
        }
        for (auto it = path.rbegin(); it != path.rend(); ++it)
        {
            for (auto &res : (*it)->resolvers)
                if (!apply_resolver(*res))
                    return false;
            c_node = std::ref(const_cast<ratio::node &>(**it));
            CURRENT_NODE(c_node);
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
    }
} // namespace ratio
