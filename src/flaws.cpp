#include "flaws.hpp"
#include "items.hpp"
#include "combinations.hpp"
#include <cassert>

namespace ratio
{
    flaw::flaw(solver &cr, std::vector<std::reference_wrapper<riddle::resolver>> &&causes) : riddle::flaw(cr, std::move(causes)), arc_consistency::listener(cr.ac_slv)
    {
        assert(cr.prop_val(get_phi()) != utils::False);
        listen(utils::variable(get_phi()));
        if (static_cast<solver &>(cr).prop_val(get_phi()) == utils::True) // The flaw is already active..
            static_cast<solver &>(cr).open_flaws.insert(this);
    }
    void flaw::on_domain_changed([[maybe_unused]] const utils::var v) noexcept
    {
        assert(v == utils::variable(get_phi()));
        if (static_cast<solver &>(gr).prop_val(get_phi()) == utils::True) // The flaw has been activated..
            static_cast<solver &>(gr).open_flaws.insert(this);
    }

    resolver::resolver(flaw &flw, const utils::lit &rho, utils::rational &&intrinsic_cost) : riddle::resolver(flw, rho, std::move(intrinsic_cost)), arc_consistency::listener(static_cast<solver &>(get_flaw().get_graph()).ac_slv)
    {
        assert(static_cast<solver &>(get_flaw().get_graph()).prop_val(rho) != utils::False);
        listen(utils::variable(rho));
        if (static_cast<solver &>(get_flaw().get_graph()).prop_val(rho) == utils::True) // The resolver is already active..
            static_cast<solver &>(get_flaw().get_graph()).open_flaws.erase(&flw);
    }

    void resolver::on_domain_changed([[maybe_unused]] const utils::var v) noexcept
    {
        assert(v == utils::variable(get_rho()));
        if (static_cast<solver &>(get_flaw().get_graph()).prop_val(get_rho()) == utils::True) // The resolver has been activated..
            static_cast<solver &>(get_flaw().get_graph()).open_flaws.erase(&flw);
    }

    enum_flaw::enum_flaw(solver &slv, std::optional<std::reference_wrapper<riddle::resolver>> cause, riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev) noexcept : flaw(slv, cause), var(std::make_shared<riddle::enum_item>(*this, tp, std::move(values), ev)) {}

    void enum_flaw::compute_resolvers()
    { // Create a resolver for each possible value..
        for (auto val : var->get_values())
        {
            auto rho = get_graph().new_bool();
            new_resolver<select_value>(*this, static_cast<riddle::bool_item &>(*rho).get_lit(), val);
        }
    }

    select_value::select_value(enum_flaw &flw, const utils::lit &rho, riddle::expr val) noexcept : resolver(flw, rho, utils::rational(1)), val(val) {}
    bool select_value::apply() noexcept { return get_flaw().get_graph().assert_expr(get_flaw().get_graph().new_eq(static_cast<enum_flaw &>(flw).get_var(), val)); }

    clause_flaw::clause_flaw(solver &slv, std::optional<std::reference_wrapper<riddle::resolver>> cause, std::vector<riddle::bool_expr> &&clause) noexcept : flaw(slv, cause), clause(std::move(clause)) {}

    void clause_flaw::compute_resolvers()
    { // Create a resolver for each literal in the clause..
        for (auto lit : clause)
            if (get_graph().bool_value(*lit) != utils::False)
                new_resolver<choose_lit>(*this, lit);
    }

    json::json clause_flaw::to_json() const
    {
        auto j = flaw::to_json();
        j["data"]["type"] = "clause";
        return j;
    }

    choose_lit::choose_lit(clause_flaw &flw, riddle::bool_expr lit) noexcept : resolver(flw, static_cast<riddle::bool_item &>(*lit).get_lit(), utils::rational(1)), lit(lit) {}
    bool choose_lit::apply() noexcept { return get_flaw().get_graph().assert_expr(get_flaw().get_graph().new_eq(lit, get_flaw().get_graph().new_bool(true))); }

    json::json choose_lit::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "lit";
        j["data"]["lit"] = lit->to_string();
        return j;
    }

    disjunction_flaw::disjunction_flaw(solver &slv, std::optional<std::reference_wrapper<riddle::resolver>> cause, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : flaw(slv, cause), disjuncts(std::move(disjuncts)) {}

    void disjunction_flaw::compute_resolvers()
    { // Create a resolver for each disjunct..
        for (const auto &disjunct : disjuncts)
            new_resolver<choose_conjunction>(*this, *disjunct);
    }

    json::json disjunction_flaw::to_json() const
    {
        auto j = flaw::to_json();
        j["data"]["type"] = "disjunction";
        return j;
    }

    choose_conjunction::choose_conjunction(disjunction_flaw &flw, riddle::conjunction &conj) noexcept : resolver(flw, utils::rational(1)), conj(conj) {}
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

    atom_flaw::atom_flaw(solver &slv, std::optional<std::reference_wrapper<riddle::resolver>> cause, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, const utils::lit &sigma) noexcept : flaw(slv, cause), atm(std::make_shared<riddle::atom>(*this, pred, is_fact, std::move(args), sigma)) {}

    void atom_flaw::compute_resolvers()
    { // Create a unify resolver for each inactive ancestor atom..
        assert(atm->get_state() == riddle::atom_state::inactive);
        for (auto &a : static_cast<riddle::predicate &>(atm->get_type()).get_atoms())
            if (atm != a && a->get_state() == riddle::active && !riddle::have_common_ancestors(static_cast<riddle::atom &>(*a).get_flaw(), static_cast<riddle::atom &>(*atm).get_flaw()) && static_cast<solver &>(get_graph()).match(*atm, *a))
                new_resolver<unify_atom>(*this, a);

        // Create an activate resolver..
        if (atm->is_fact())
            if (get_resolvers().empty())
                new_resolver<activate_fact>(*this, get_phi());
            else
                new_resolver<activate_fact>(*this);
        else if (get_resolvers().empty())
            new_resolver<activate_goal>(*this, get_phi());
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

    activate_fact::activate_fact(atom_flaw &flw) noexcept : resolver(flw, utils::rational(1)) {}
    activate_fact::activate_fact(atom_flaw &flw, const utils::lit &rho) noexcept : resolver(flw, rho, utils::rational(1)) {}
    bool activate_fact::apply() noexcept
    {
        static_cast<solver &>(get_flaw().get_graph()).new_clause({!get_rho(), static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma()});
        return true;
    }

    json::json activate_fact::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "activate_fact";
        return j;
    }

    activate_goal::activate_goal(atom_flaw &flw) noexcept : resolver(flw, utils::rational(1)) {}
    activate_goal::activate_goal(atom_flaw &flw, const utils::lit &rho) noexcept : resolver(flw, rho, utils::rational(1)) {}
    bool activate_goal::apply() noexcept
    {
        static_cast<solver &>(get_flaw().get_graph()).new_clause({!get_rho(), static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma()});
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

    unify_atom::unify_atom(atom_flaw &flw, riddle::atom_expr atm) noexcept : resolver(flw, utils::rational(0)), atm(std::move(atm)) {}
    bool unify_atom::apply() noexcept
    {
        get_flaw().get_graph().add_causal_link(static_cast<riddle::atom &>(*atm).get_flaw(), *this);
        static_cast<solver &>(get_flaw().get_graph()).new_clause({!get_rho(), static_cast<riddle::atom &>(*atm).get_sigma()});
        static_cast<solver &>(get_flaw().get_graph()).new_clause({!get_rho(), !static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma()});
        return get_flaw().get_graph().assert_expr(get_flaw().get_graph().new_eq(static_cast<atom_flaw &>(flw).get_atom(), atm));
    }

    json::json unify_atom::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "unify_atom";
        j["data"]["atom_id"] = atm->get_id();
        return j;
    }

    sv_peak::sv_peak(solver &slv, std::vector<riddle::atom_expr> &&atms) noexcept : flaw(slv, riddle::causes_from_atoms(atms)), atms(std::move(atms)) {}
    void sv_peak::compute_resolvers() noexcept
    {
        for (const auto &as : utils::combinations(std::vector<riddle::atom_expr>(atms.cbegin(), atms.cend()), 2))
        {
            new_resolver<ordering>(*this, as[0], as[1]);
            new_resolver<ordering>(*this, as[1], as[0]);
        }
    }

    rr_peak::rr_peak(solver &slv, std::vector<riddle::atom_expr> &&atms) noexcept : flaw(slv, riddle::causes_from_atoms(atms)), atms(std::move(atms)) {}
    void rr_peak::compute_resolvers() noexcept
    {
        for (const auto &as : utils::combinations(std::vector<riddle::atom_expr>(atms.cbegin(), atms.cend()), 2))
        {
            new_resolver<ordering>(*this, as[0], as[1]);
            new_resolver<ordering>(*this, as[1], as[0]);
        }
    }

    std::vector<std::reference_wrapper<riddle::resolver>> merge_vectors(std::vector<std::reference_wrapper<riddle::resolver>> &&a, const std::vector<std::reference_wrapper<riddle::resolver>> &&b)
    {
        a.insert(a.end(), b.begin(), b.end());
        return a;
    }

    cr_overproduction::cr_overproduction(solver &slv, std::vector<riddle::atom_expr> &&prod_atms, std::vector<riddle::atom_expr> &&cons_atms) noexcept : flaw(slv, merge_vectors(riddle::causes_from_atoms(prod_atms), riddle::causes_from_atoms(cons_atms))), prod_atms(std::move(prod_atms)), cons_atms(std::move(cons_atms)) {}
    void cr_overproduction::compute_resolvers() noexcept
    {
        for (const auto &p : prod_atms)
            for (const auto &c : cons_atms)
                new_resolver<ordering>(*this, p, c);
    }

    cr_overconsumption::cr_overconsumption(solver &slv, std::vector<riddle::atom_expr> &&cons_atms, std::vector<riddle::atom_expr> &&prod_atms) noexcept : flaw(slv, merge_vectors(riddle::causes_from_atoms(cons_atms), riddle::causes_from_atoms(prod_atms))), cons_atms(std::move(cons_atms)), prod_atms(std::move(prod_atms)) {}
    void cr_overconsumption::compute_resolvers() noexcept
    {
        for (const auto &c : cons_atms)
            for (const auto &p : prod_atms)
                new_resolver<ordering>(*this, c, p);
    }

    ordering::ordering(flaw &flw, riddle::atom_expr before, riddle::atom_expr after) noexcept : resolver(flw, utils::rational(1)), before(std::move(before)), after(std::move(after)) {}
    bool ordering::apply() noexcept { return get_flaw().get_graph().assert_expr(get_flaw().get_graph().new_le(before->get<riddle::arith_term>(riddle::end_kw), after->get<riddle::arith_term>(riddle::start_kw))); }
} // namespace ratio
