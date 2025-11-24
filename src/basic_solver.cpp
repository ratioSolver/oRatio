#include "basic_solver.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
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
                compute_resolvers(flw);
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

    enum_flaw::enum_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::enum_expr var) noexcept : flaw(slv, std::move(causes)), var(std::move(var)) {}

    void enum_flaw::compute_resolvers()
    { // Create a resolver for each possible value..
        for (auto &val : var->get_values())
            get_solver().new_resolver<choose_val>(*this, val);
    }

    choose_val::choose_val(enum_flaw &f, riddle::expr val) noexcept : resolver(f, utils::rational(1)), val(std::move(val)) {}
    void choose_val::apply()
    {
        auto &e_item = static_cast<riddle::enum_item &>(*static_cast<enum_flaw &>(flw).get_var());
        add_ac_constraint(get_ac().new_assign(utils::variable(e_item.get_var()), static_cast<utils::enum_val &>(*val)));
    }

    clause_flaw::clause_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause) noexcept : flaw(slv, std::move(causes)), clause(std::move(clause)) {}

    void clause_flaw::compute_resolvers()
    { // Create a resolver for each literal in the clause..
        for (const auto &lit : clause)
            get_solver().new_resolver<choose_lit>(*this, lit);
    }

    choose_lit::choose_lit(clause_flaw &f, riddle::bool_expr lit) noexcept : resolver(f, utils::rational(1)), lit(lit) {}
    void choose_lit::apply()
    {
        auto &c_lit = static_cast<const riddle::bool_item &>(*lit).get_lit();
        add_ac_constraint(get_ac().new_assign(utils::variable(c_lit), utils::sign(c_lit) ? arc_consistency::solver::True : arc_consistency::solver::False));
    }

    disjunction_flaw::disjunction_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    void disjunction_flaw::compute_resolvers()
    { // Create a resolver for each disjunct..
        for (const auto &disjunct : disjuncts)
            get_solver().new_resolver<choose_conjunction>(*this, *disjunct);
    }

    choose_conjunction::choose_conjunction(disjunction_flaw &f, riddle::conjunction &conj) noexcept : resolver(f, utils::rational(1)), conj(conj) {}
    void choose_conjunction::apply() { conj.execute(); }

    atom_flaw::atom_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept : flaw(slv, std::move(causes)), atm(std::make_shared<atom>(pred, is_fact, std::move(args), std::move(sigma), *this)) {}

    void atom_flaw::compute_resolvers()
    { // Create a unify resolver for each inactive ancestor atom..
        assert(atm->get_state() == riddle::atom_state::inactive);
        for (auto &a : static_cast<riddle::predicate &>(atm->get_type()).get_atoms())
            if (static_cast<atom &>(*a).get_flaw().is_expanded() && !have_common_ancestors(a, atm) && get_solver().match(*atm, *a))
                get_solver().new_resolver<unify_atom>(*this, a);

        // Create an activate resolver..
        if (atm->is_fact())
            get_solver().new_resolver<activate_fact>(*this);
        else
            get_solver().new_resolver<activate_goal>(*this);
    }

    bool atom_flaw::have_common_ancestors(const riddle::atom_expr &ancestor, const riddle::atom_expr &descendant)
    {
        flaw *curr_f = &static_cast<atom &>(*descendant).get_flaw();
        std::unordered_set<flaw *> visited;
        while (curr_f)
        {
            visited.insert(curr_f);
            if (curr_f == &static_cast<atom &>(*ancestor).get_flaw())
                return true;
            curr_f = curr_f->get_causes().empty() ? nullptr : &curr_f->get_causes().front().get().get_flaw();
        }

        flaw *anc_f = &static_cast<atom &>(*ancestor).get_flaw();
        while (anc_f)
        {
            if (visited.count(anc_f))
                return true;
            anc_f = anc_f->get_causes().empty() ? nullptr : &anc_f->get_causes().front().get().get_flaw();
        }
        return false;
    }

    json::json atom_flaw::to_json() const
    {
        auto j = flaw::to_json();
        j["data"]["type"] = "atom";
        j["data"]["atom"] = {{"id", atm->get_id()}, {"is_fact", atm->is_fact()}, {"predicate", atm->get_type().get_name()}};
        return j;
    }

    activate_fact::activate_fact(atom_flaw &f) noexcept : resolver(f, utils::rational(1)) {}
    void activate_fact::apply() { add_ac_constraint(get_ac().new_assign(utils::variable(static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma()), arc_consistency::solver::True)); }

    json::json activate_fact::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "activate_fact";
        return j;
    }

    activate_goal::activate_goal(atom_flaw &f) noexcept : resolver(f, utils::rational(1)) {}
    void activate_goal::apply()
    {
        add_ac_constraint(get_ac().new_assign(utils::variable(static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma()), arc_consistency::solver::True));
        static_cast<riddle::predicate &>(static_cast<atom_flaw &>(flw).get_atom()->get_type()).call(static_cast<atom_flaw &>(flw).get_atom());
    }

    json::json activate_goal::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "activate_goal";
        return j;
    }

    unify_atom::unify_atom(atom_flaw &f, riddle::atom_expr atm) noexcept : resolver(f, utils::rational(2)), atm(std::move(atm)) {}
    void unify_atom::apply()
    {
        execute(get_solver().new_eq(static_cast<atom_flaw &>(flw).get_atom(), atm));
        add_ac_constraint(get_ac().new_assign(utils::variable(static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma()), arc_consistency::solver::False));
    }

    json::json unify_atom::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "unify_atom";
        j["data"]["atom_id"] = atm->get_id();
        return j;
    }
} // namespace ratio
