#include "basic_flaws.hpp"
#include "basic_solver.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <stack>
#include <cassert>

namespace ratio
{
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

    json::json clause_flaw::to_json() const
    {
        auto j = flaw::to_json();
        j["data"]["type"] = "clause";
        return j;
    }

    choose_lit::choose_lit(clause_flaw &f, riddle::bool_expr lit) noexcept : resolver(f, utils::rational(1)), lit(lit) {}
    void choose_lit::apply()
    {
        auto &c_lit = static_cast<const riddle::bool_item &>(*lit).get_lit();
        add_ac_constraint(get_ac().new_assign(utils::variable(c_lit), utils::sign(c_lit) ? arc_consistency::solver::True : arc_consistency::solver::False));
    }

    json::json choose_lit::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "lit";
        j["data"]["lit"] = to_string(static_cast<const riddle::bool_item &>(*lit).get_lit());
        return j;
    }

    disjunction_flaw::disjunction_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    void disjunction_flaw::compute_resolvers()
    { // Create a resolver for each disjunct..
        for (const auto &disjunct : disjuncts)
            get_solver().new_resolver<choose_conjunction>(*this, *disjunct);
    }

    json::json disjunction_flaw::to_json() const
    {
        auto j = flaw::to_json();
        j["data"]["type"] = "disjunction";
        return j;
    }

    choose_conjunction::choose_conjunction(disjunction_flaw &f, riddle::conjunction &conj) noexcept : resolver(f, utils::rational(1)), conj(conj) {}
    void choose_conjunction::apply() { conj.execute(); }

    json::json choose_conjunction::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "disjunct";
        return j;
    }

    atom_flaw::atom_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept : flaw(slv, std::move(causes)), atm(std::make_shared<atom>(pred, is_fact, std::move(args), std::move(sigma), *this)) {}

    void atom_flaw::compute_resolvers()
    { // Create a unify resolver for each inactive ancestor atom..
        assert(atm->get_state() == riddle::atom_state::inactive);
        for (auto &a : static_cast<riddle::predicate &>(atm->get_type()).get_atoms())
            if (static_cast<atom &>(*a).get_flaw().is_expanded() && !have_common_ancestors(a, atm)) // we only consider expanded ancestor atoms that do not share common ancestors with the current atom (we can't filter out unifications because we are building flaws within a search tree and we are sharing them across different branches)..
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
        get_solver().add_causal_link(static_cast<atom &>(*atm).get_flaw(), *this);
    }

    json::json unify_atom::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "unify_atom";
        j["data"]["atom_id"] = atm->get_id();
        return j;
    }
} // namespace ratio
