#include "basic_flaws.hpp"
#include "items.hpp"
#include "combinations.hpp"
#include <cmath>
#include <cassert>

namespace ratio
{
    enum_flaw::enum_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev) noexcept : flaw(slv, std::move(causes)), var(std::make_shared<riddle::enum_item>(*this, tp, std::move(values), ev)) {}

    utils::rational enum_flaw::get_estimated_cost() const noexcept { return get_core().enum_value(*var).size(); }

    void enum_flaw::compute_resolvers()
    {
        if (expanded)
            return;
        // Create a resolver for each possible value..
        for (auto val : var->get_values())
            new_resolver<select_value>(*this, val);
        expanded = true;
    }

    select_value::select_value(enum_flaw &flw, riddle::expr val) noexcept : riddle::resolver(flw, utils::rational(1)), resolver(flw, utils::rational(1)), riddle::select_value(flw, std::move(val)) {}
    bool select_value::apply() noexcept { return ratio::resolver::get_flaw().get_core().assert_expr(ratio::resolver::get_flaw().get_core().new_eq(static_cast<enum_flaw &>(ratio::resolver::flw).get_var(), get_value())); }

    clause_flaw::clause_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, std::vector<riddle::bool_expr> &&clause) noexcept : flaw(slv, std::move(causes)), clause(std::move(clause)) {}

    utils::rational clause_flaw::get_estimated_cost() const noexcept { return clause.size(); }

    void clause_flaw::compute_resolvers()
    { // Create a resolver for each literal in the clause..
        clear_resolvers();
        for (auto lit : clause)
            if (get_core().bool_value(*lit) != utils::False)
                new_resolver<choose_lit>(*this, lit);
    }

    json::json clause_flaw::to_json() const
    {
        auto j = flaw::to_json();
        j["data"]["type"] = "clause";
        return j;
    }

    choose_lit::choose_lit(clause_flaw &flw, riddle::bool_expr lit) noexcept : riddle::resolver(flw, utils::rational(1)), resolver(flw, utils::rational(1)), lit(lit) {}
    bool choose_lit::apply() noexcept { return ratio::resolver::get_flaw().get_core().assert_expr(ratio::resolver::get_flaw().get_core().new_eq(lit, ratio::resolver::get_flaw().get_core().new_bool(true))); }

    json::json choose_lit::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "lit";
        j["data"]["lit"] = lit->to_string();
        return j;
    }

    disjunction_flaw::disjunction_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    utils::rational disjunction_flaw::get_estimated_cost() const noexcept { return disjuncts.size(); }

    void disjunction_flaw::compute_resolvers()
    { // Create a resolver for each disjunct..
        clear_resolvers();
        for (const auto &disjunct : disjuncts)
            new_resolver<choose_conjunction>(*this, *disjunct);
    }

    json::json disjunction_flaw::to_json() const
    {
        auto j = flaw::to_json();
        j["data"]["type"] = "disjunction";
        return j;
    }

    choose_conjunction::choose_conjunction(disjunction_flaw &flw, riddle::conjunction &conj) noexcept : riddle::resolver(flw, utils::rational(1)), resolver(flw, utils::rational(1)), conj(conj) {}
    bool choose_conjunction::apply() noexcept
    {
        conj.execute();
        return true;
    }

    json::json choose_conjunction::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "disjunct";
        return j;
    }

    atom_flaw::atom_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, riddle::bool_expr &&sigma) noexcept : flaw(slv, std::move(causes)), atm(std::make_shared<riddle::atom>(*this, pred, is_fact, std::move(args), std::move(sigma))) {}

    utils::rational atom_flaw::get_estimated_cost() const noexcept
    { // Estimate the cost as the number of active ancestor atoms that can be unified with this atom plus one for activation..
        if (atm->is_fact())
            return utils::rational::zero; // activating a fact has zero cost
        std::size_t count = 0;
        for (auto &a : static_cast<riddle::predicate &>(atm->get_type()).get_atoms())
            if (atm != a && a->get_state() == riddle::active && !have_common_ancestors(a->get_flaw(), atm->get_flaw()) && static_cast<solver &>(get_core()).match(*atm, *a))
                ++count;
        return utils::rational(count + 1);
    }

    void atom_flaw::compute_resolvers()
    { // Create a unify resolver for each inactive ancestor atom..
        clear_resolvers();
        assert(atm->get_state() == riddle::atom_state::inactive);
        for (auto &a : static_cast<riddle::predicate &>(atm->get_type()).get_atoms())
            if (atm != a && a->get_state() == riddle::active && !have_common_ancestors(a->get_flaw(), atm->get_flaw()) && static_cast<solver &>(get_core()).match(*atm, *a))
                new_resolver<unify_atom>(*this, a);

        // Create an activate resolver..
        if (atm->is_fact())
            new_resolver<activate_fact>(*this);
        else
            new_resolver<activate_goal>(*this);
    }

    json::json atom_flaw::to_json() const
    {
        auto j = flaw::to_json();
        j["data"]["type"] = "atom";
        j["data"]["atom"] = {{"id", atm->get_id()}, {"is_fact", atm->is_fact()}, {"predicate", atm->get_type().get_name()}};
        return j;
    }

    activate_fact::activate_fact(atom_flaw &flw) noexcept : riddle::resolver(flw, utils::rational(1)), resolver(flw, utils::rational(1)) {}
    bool activate_fact::apply() noexcept { return ratio::resolver::get_flaw().get_core().assert_expr(static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma()); }

    json::json activate_fact::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "activate_fact";
        return j;
    }

    activate_goal::activate_goal(atom_flaw &flw) noexcept : riddle::resolver(flw, utils::rational(1)), resolver(flw, utils::rational(1)) {}
    bool activate_goal::apply() noexcept
    {
        if (!ratio::resolver::get_flaw().get_core().assert_expr(static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma()))
            return false;
        try
        {
            static_cast<riddle::predicate &>(static_cast<atom_flaw &>(flw).get_atom()->get_type()).call(static_cast<atom_flaw &>(flw).get_atom());
            return true;
        }
        catch (const std::exception &e)
        {
            return false;
        }
    }

    json::json activate_goal::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "activate_goal";
        return j;
    }

    unify_atom::unify_atom(atom_flaw &flw, riddle::atom_expr atm) noexcept : riddle::resolver(flw, utils::rational(0)), resolver(flw, utils::rational(0)), atm(std::move(atm)) {}
    bool unify_atom::apply() noexcept
    {
        if (!ratio::resolver::get_flaw().get_core().assert_expr(static_cast<riddle::atom &>(*atm).get_sigma()))
            return false;
        if (!ratio::resolver::get_flaw().get_core().assert_expr(ratio::resolver::get_flaw().get_core().new_not(static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma())))
            return false;
        return ratio::resolver::get_flaw().get_core().assert_expr(ratio::resolver::get_flaw().get_core().new_eq(static_cast<atom_flaw &>(flw).get_atom(), atm));
    }

    json::json unify_atom::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "unify_atom";
        j["data"]["atom_id"] = atm->get_id();
        return j;
    }

    sv_peak::sv_peak(basic_solver &slv, std::vector<riddle::atom_expr> &&atms) noexcept : riddle::flaw(slv, riddle::causes_from_atoms(atms)), atms(std::move(atms)) {}
    utils::rational sv_peak::get_estimated_cost() const noexcept { return std::pow(2.0, static_cast<double>(get_causes().size() - 1)); }
    void sv_peak::compute_resolvers() noexcept
    {
        for (const auto &as : utils::combinations(std::vector<riddle::atom_expr>(atms.cbegin(), atms.cend()), 2))
        {
            new_resolver<ordering>(*this, as[0], as[1]);
            new_resolver<ordering>(*this, as[1], as[0]);
        }
    }

    rr_peak::rr_peak(basic_solver &slv, std::vector<riddle::atom_expr> &&atms) noexcept : riddle::flaw(slv, riddle::causes_from_atoms(atms)), atms(std::move(atms)) {}
    utils::rational rr_peak::get_estimated_cost() const noexcept { return std::pow(2.0, static_cast<double>(get_causes().size() - 1)); }
    void rr_peak::compute_resolvers() noexcept
    {
        for (const auto &as : utils::combinations(std::vector<riddle::atom_expr>(atms.cbegin(), atms.cend()), 2))
        {
            new_resolver<ordering>(*this, as[0], as[1]);
            new_resolver<ordering>(*this, as[1], as[0]);
        }
    }

    std::vector<std::shared_ptr<riddle::resolver>> merge_vectors(std::vector<std::shared_ptr<riddle::resolver>> &&a, const std::vector<std::shared_ptr<riddle::resolver>> &&b)
    {
        a.insert(a.end(), b.begin(), b.end());
        return a;
    }

    cr_overproduction::cr_overproduction(basic_solver &slv, std::vector<riddle::atom_expr> &&prod_atms, std::vector<riddle::atom_expr> &&cons_atms) noexcept : riddle::flaw(slv, merge_vectors(riddle::causes_from_atoms(prod_atms), riddle::causes_from_atoms(cons_atms))), prod_atms(std::move(prod_atms)), cons_atms(std::move(cons_atms)) {}
    utils::rational cr_overproduction::get_estimated_cost() const noexcept { return utils::rational(prod_atms.size() * cons_atms.size()); }
    void cr_overproduction::compute_resolvers() noexcept
    {
        for (const auto &p : prod_atms)
            for (const auto &c : cons_atms)
                new_resolver<ordering>(*this, p, c);
    }

    cr_overconsumption::cr_overconsumption(basic_solver &slv, std::vector<riddle::atom_expr> &&cons_atms, std::vector<riddle::atom_expr> &&prod_atms) noexcept : riddle::flaw(slv, merge_vectors(riddle::causes_from_atoms(cons_atms), riddle::causes_from_atoms(prod_atms))), cons_atms(std::move(cons_atms)), prod_atms(std::move(prod_atms)) {}
    utils::rational cr_overconsumption::get_estimated_cost() const noexcept { return utils::rational(cons_atms.size() * prod_atms.size()); }
    void cr_overconsumption::compute_resolvers() noexcept
    {
        for (const auto &c : cons_atms)
            for (const auto &p : prod_atms)
                new_resolver<ordering>(*this, c, p);
    }

    ordering::ordering(riddle::flaw &flw, riddle::atom_expr before, riddle::atom_expr after) noexcept : riddle::resolver(flw, utils::rational(1)), resolver(flw, utils::rational(1)), before(std::move(before)), after(std::move(after)) {}
    bool ordering::apply() noexcept { return ratio::resolver::get_flaw().get_core().assert_expr(ratio::resolver::get_flaw().get_core().new_le(before->get<riddle::arith_term>(riddle::end_kw), after->get<riddle::arith_term>(riddle::start_kw))); }
} // namespace ratio
