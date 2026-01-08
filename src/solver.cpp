#include "solver.hpp"
#include "items.hpp"
#include "flaws.hpp"
#include "logging.hpp"
#include <cassert>

#ifdef ORATIO_ENABLE_LISTENERS
#define STATE_CHANGED() state_changed()
#define FLAW_CREATED(f) flaw_created(f)
#define FLAW_STATE_CHANGED(f) flaw_state_changed(f)
#define FLAW_COST_CHANGED(f) flaw_cost_changed(f)
#define RESOLVER_CREATED(r) resolver_created(r)
#define RESOLVER_STATE_CHANGED(r) resolver_state_changed(r)
#define NEW_CAUSAL_LINK(f, r) causal_link_added(f, r)
#define CURRENT_FLAW(f) current_flaw(f)
#define CURRENT_RESOLVER(r) current_resolver(r)
#else
#define STATE_CHANGED()
#define FLAW_CREATED(f)
#define FLAW_STATE_CHANGED(f)
#define FLAW_COST_CHANGED(f)
#define RESOLVER_CREATED(r)
#define RESOLVER_STATE_CHANGED(r)
#define NEW_CAUSAL_LINK(f, r)
#define CURRENT_FLAW(f)
#define CURRENT_RESOLVER(r)
#endif

namespace ratio
{
    solver::solver(std::string_view name) noexcept : riddle::core(name)
    {
        read(INIT_STRING);

        add_type(std::make_unique<state_variable>(*this));
        add_type(std::make_unique<reusable_resource>(*this));
        add_type(std::make_unique<consumable_resource>(*this));
    }

    riddle::bool_expr solver::new_bool() { return std::make_shared<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), ac_slv.new_sat()); }
    riddle::bool_expr solver::new_bool(const bool value)
    {
        auto l = value ? utils::TRUE_lit : utils::FALSE_lit;
        return std::make_shared<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::move(l));
    }
    utils::lbool solver::bool_value(const riddle::bool_term &expr) const noexcept { return ac_slv.sat_val(static_cast<const riddle::bool_item &>(expr).get_lit()); }

    riddle::arith_expr solver::new_int() { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(lin_slv.new_var(), utils::rational::one)); }
    riddle::arith_expr solver::new_int(const INT_TYPE value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::rational(value)); }
    riddle::arith_expr solver::new_int(const INT_TYPE lb, const INT_TYPE ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), lin_slv.new_var(utils::rational(lb), utils::rational(ub))); }
    riddle::arith_expr solver::new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), lin_slv.new_var(utils::rational(lb), utils::rational(ub))); }

    riddle::arith_expr solver::new_real() { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(lin_slv.new_var(), utils::rational::one)); }
    riddle::arith_expr solver::new_real(utils::rational &&value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(value)); }
    riddle::arith_expr solver::new_real(utils::rational &&lb, utils::rational &&ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), lin_slv.new_var(std::move(lb), std::move(ub))); }
    riddle::arith_expr solver::new_uncertain_real(utils::rational &&lb, utils::rational &&ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), lin_slv.new_var(std::move(lb), std::move(ub))); }

    riddle::arith_expr solver::new_time() { return std::make_shared<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), utils::lin(lin_slv.new_var(), utils::rational::one)); }
    riddle::arith_expr solver::new_time(utils::rational &&value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), std::move(value)); }

    utils::inf_rational solver::arith_value(const riddle::arith_term &xpr) const noexcept { return lin_slv.val(static_cast<const riddle::arith_item &>(xpr).get_lin()); }
    bool solver::is_constant(const riddle::arith_term &xpr) const noexcept { return lin_slv.lb(static_cast<const riddle::arith_item &>(xpr).get_lin()) == lin_slv.ub(static_cast<const riddle::arith_item &>(xpr).get_lin()); }

    riddle::string_expr solver::new_string() { return std::make_shared<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ""); }
    riddle::string_expr solver::new_string(std::string &&value) { return std::make_shared<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), std::move(value)); }
    std::string solver::string_value(const riddle::string_term &xpr) const noexcept { return static_cast<const riddle::string_item &>(xpr).get_string(); }

    std::unordered_set<riddle::expr> solver::enum_value(const riddle::enum_term &xpr) const noexcept
    {
        auto &dom = ac_slv.domain(static_cast<const riddle::enum_item &>(xpr).get_var());
        std::unordered_set<riddle::expr> values;
        for (auto ev_ptr : xpr.get_values())
            if (dom.find(&*ev_ptr) != dom.end())
                values.insert(ev_ptr);
        return values;
    };

    riddle::arith_expr solver::new_negation(riddle::arith_expr xpr)
    {
        if (xpr->get_type().get_name() == riddle::int_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), -static_cast<const riddle::arith_item &>(*xpr).get_lin());
        else if (xpr->get_type().get_name() == riddle::real_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), -static_cast<const riddle::arith_item &>(*xpr).get_lin());
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr solver::new_sum(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin sum;
        for (const auto &xpr : xprs)
            sum += static_cast<const riddle::arith_item &>(*xpr).get_lin();
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(sum));
        else if (tp.get_name() == riddle::real_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(sum));
        else
            throw std::runtime_error("Invalid type");
    }
    riddle::arith_expr solver::new_subtraction(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin sub = static_cast<const riddle::arith_item &>(*xprs[0]).get_lin();
        for (size_t i = 1; i < xprs.size(); i++)
            sub -= static_cast<const riddle::arith_item &>(*xprs[i]).get_lin();
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(sub));
        else if (tp.get_name() == riddle::real_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(sub));
        else
            throw std::runtime_error("Invalid type");
    }
    riddle::arith_expr solver::new_product(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin prod;
        for (const auto &xpr : xprs)
            if (static_cast<const riddle::arith_item &>(*xpr).get_lin().vars.empty())
                prod *= static_cast<const riddle::arith_item &>(*xpr).get_lin().known_term;
            else
                throw std::runtime_error("Non-linear arithmetic not supported");
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(prod));
        else if (tp.get_name() == riddle::real_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(prod));
        else
            throw std::runtime_error("Invalid type");
    }
    riddle::arith_expr solver::new_division(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin div = static_cast<const riddle::arith_item &>(*xprs[0]).get_lin();
        for (size_t i = 1; i < xprs.size(); i++)
            if (static_cast<const riddle::arith_item &>(*xprs[i]).get_lin().vars.empty())
                div /= static_cast<const riddle::arith_item &>(*xprs[i]).get_lin().known_term;
            else
                throw std::runtime_error("Non-linear arithmetic not supported");
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(div));
        else if (tp.get_name() == riddle::real_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(div));
        else
            throw std::runtime_error("Invalid type");
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
        { // Multi-valued enum
            std::vector<std::reference_wrapper<const utils::enum_val>> ev_refs;
            for (auto &ev_ptr : values)
                ev_refs.emplace_back(*ev_ptr);
            auto ev = ac_slv.new_var(ev_refs);
            // .. and create a new enum flaw to manage the variable..
            std::vector<std::shared_ptr<riddle::resolver>> causes;
            auto res = get_current_resolver();
            if (res)
                causes.push_back(res);
            auto ef = new_flaw<enum_flaw>(*this, std::move(causes), tp, std::move(values), ev);
            FLAW_CREATED(*ef);
            return ef->get_var();
        }
    }

    void solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<std::shared_ptr<riddle::resolver>> causes;
        auto res = get_current_resolver();
        if (res)
            causes.push_back(res);
        [[maybe_unused]] auto df = new_flaw<disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
        FLAW_CREATED(*df);
    }
    void solver::new_clause(std::vector<riddle::bool_expr> &&exprs)
    {
        assert(!exprs.empty());
        if (exprs.size() == 1)
        { // if there is only one expression, just execute it..
            if (!assert_expr(exprs[0]))
                throw std::runtime_error("Unsatisfiable constraints");
        }
        else
        { // otherwise, create a new clause flaw..
            std::vector<std::shared_ptr<riddle::resolver>> causes;
            auto res = get_current_resolver();
            if (res)
                causes.push_back(res);
            [[maybe_unused]] auto cf = new_flaw<clause_flaw>(*this, std::move(causes), std::move(exprs));
            FLAW_CREATED(*cf);
        }
    }

    bool solver::match(riddle::term &lhs, riddle::term &rhs) const
    {
        if (&lhs == &rhs) // the terms are the same, so they match..
            return true;
        else if (&lhs.get_type() != &rhs.get_type()) // the types are different, so the terms cannot match..
            return false;
        else if (auto lhs_xpr = dynamic_cast<riddle::arith_item *>(&lhs)) // we are dealing with arithmetic terms..
            return lin_slv.match(lhs_xpr->get_lin(), static_cast<riddle::arith_item &>(rhs).get_lin());
        else if (auto lhs_bxpr = dynamic_cast<riddle::bool_item *>(&lhs)) // we are dealing with boolean terms..
            return ac_slv.match(lhs_bxpr->get_lit(), static_cast<riddle::bool_item &>(rhs).get_lit());
        else if (auto lhs_enum_xpr = dynamic_cast<riddle::enum_item *>(&lhs)) // we are dealing with enum terms..
        {
            if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(&rhs))
                return ac_slv.match(lhs_enum_xpr->get_var(), rhs_enum_xpr->get_var());
            else
                return ac_slv.allows(lhs_enum_xpr->get_var(), rhs);
        }
        else if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(&rhs)) // we are dealing with enum terms..
            return ac_slv.allows(rhs_enum_xpr->get_var(), lhs);
        else if (auto lhs_xpr = dynamic_cast<riddle::atom_term *>(&lhs))
        { // we are dealing with atoms..
            auto rhs_xpr = static_cast<riddle::atom_term *>(&rhs);
            if (&lhs_xpr->get_type().get_scope() != &rhs_xpr->get_type().get_scope().get_core() && !match(*lhs_xpr->get(riddle::tau_kw), *rhs_xpr->get(riddle::tau_kw)))
                return false; // the atoms are not in the same scope, so they cannot match..
            // we check if the atoms' fields match..
            std::queue<riddle::predicate *> q;
            q.push(static_cast<riddle::predicate *>(&lhs_xpr->get_type()));
            while (!q.empty())
            {
                for (const auto &[f_name, f] : q.front()->get_fields())
                    if (!match(*lhs_xpr->get(f_name), *rhs_xpr->get(f_name)))
                        return false;
                for (const auto &pp : q.front()->get_parents())
                    q.push(&pp.get());
                q.pop();
            }
            return true;
        }
        else // we are dealing with components (and we have already checked their are not the same)..
            return false;
    }

    void solver::solve()
    {
        size_t iter = 0;
        while (iter < get_flaws().size())
        {
            auto &flw = get_flaws()[iter];
            set_current_flaw(flw);
            if (flw->get_resolvers().empty())
            {
                compute_resolvers(*flw);
                for (const auto &r : flw->get_resolvers())
                {
                    set_current_resolver(r);
                    apply_resolver(r, sat_val(static_cast<const flaw &>(*flw).get_phi()) != utils::True || flw->get_resolvers().size() > 1);
                }
                if (!lin_slv.check() || !ac_slv.propagate())
                { // TODO: unsat handling
                }
            }
            iter++;
        }
    }

    void solver::new_clause(std::vector<utils::lit> &&lits) { ac_slv.add_constraint(ac_slv.new_clause(std::move(lits))); }
    utils::lbool solver::sat_val(const utils::lit &l) const noexcept { return ac_slv.sat_val(l); }

    riddle::atom_expr solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::term>, std::less<>> &&args)
    {
        std::vector<std::shared_ptr<riddle::resolver>> causes;
        auto res = get_current_resolver();
        if (res)
            causes.push_back(res);
        auto af = new_flaw<atom_flaw>(*this, std::move(causes), is_fact, pred, std::move(args), new_bool());
        FLAW_CREATED(*af);
        return af->get_atom();
    }

    riddle::atom_state solver::get_atom_state(const riddle::atom_term &atm) const noexcept
    {
        switch (bool_value(*static_cast<const riddle::atom &>(atm).get_sigma()))
        {
        case utils::True:
            return riddle::active;
        case utils::False:
            return riddle::unified;
        default:
            return riddle::inactive;
        }
    }

    bool solver::mk_assign(riddle::bool_expr xpr, utils::lbool val) noexcept
    {
        auto &c = ac_slv.new_assign(utils::variable(std::static_pointer_cast<const riddle::bool_item>(xpr)->get_lit()), val ? arc_consistency::solver::True : arc_consistency::solver::False);
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            ac_slv.add_constraint(c);
        else
            dynamic_cast<resolver &>(*get_current_resolver()).ac_cnsts.push_back(c);
        return true;
    }
    bool solver::mk_eq(riddle::bool_expr lhs, riddle::bool_expr rhs) noexcept
    {
        auto &c = ac_slv.new_equal(utils::variable(std::static_pointer_cast<const riddle::bool_item>(lhs)->get_lit()), utils::variable(std::static_pointer_cast<const riddle::bool_item>(rhs)->get_lit()));
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            ac_slv.add_constraint(c);
        else
            dynamic_cast<resolver &>(*get_current_resolver()).ac_cnsts.push_back(c);
        return true;
    }
    bool solver::mk_neq(riddle::bool_expr lhs, riddle::bool_expr rhs) noexcept
    {
        auto &c = ac_slv.new_distinct(utils::variable(std::static_pointer_cast<const riddle::bool_item>(lhs)->get_lit()), utils::variable(std::static_pointer_cast<const riddle::bool_item>(rhs)->get_lit()));
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            ac_slv.add_constraint(c);
        else
            dynamic_cast<resolver &>(*get_current_resolver()).ac_cnsts.push_back(c);
        return true;
    }

    bool solver::mk_lt(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept
    {
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            return lin_slv.new_lt(std::static_pointer_cast<const riddle::arith_item>(lhs)->get_lin(), std::static_pointer_cast<const riddle::arith_item>(rhs)->get_lin(), true);
        else
            return lin_slv.new_lt(std::static_pointer_cast<const riddle::arith_item>(lhs)->get_lin(), std::static_pointer_cast<const riddle::arith_item>(rhs)->get_lin(), true, dynamic_cast<resolver &>(*get_current_resolver()).lin_cnsts);
    }
    bool solver::mk_le(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept
    {
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            return lin_slv.new_lt(std::static_pointer_cast<const riddle::arith_item>(lhs)->get_lin(), std::static_pointer_cast<const riddle::arith_item>(rhs)->get_lin(), false);
        else
            return lin_slv.new_lt(std::static_pointer_cast<const riddle::arith_item>(lhs)->get_lin(), std::static_pointer_cast<const riddle::arith_item>(rhs)->get_lin(), false, dynamic_cast<resolver &>(*get_current_resolver()).lin_cnsts);
    }
    bool solver::mk_eq(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept
    {
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            return lin_slv.new_eq(std::static_pointer_cast<const riddle::arith_item>(lhs)->get_lin(), std::static_pointer_cast<const riddle::arith_item>(rhs)->get_lin());
        else
            return lin_slv.new_eq(std::static_pointer_cast<const riddle::arith_item>(lhs)->get_lin(), std::static_pointer_cast<const riddle::arith_item>(rhs)->get_lin(), dynamic_cast<resolver &>(*get_current_resolver()).lin_cnsts);
    }
    bool solver::mk_neq(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept
    {
        new_clause({new_lt(lhs, rhs), new_lt(rhs, lhs)});
        return true;
    }
    bool solver::mk_ge(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept
    {
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            return lin_slv.new_gt(std::static_pointer_cast<const riddle::arith_item>(lhs)->get_lin(), std::static_pointer_cast<const riddle::arith_item>(rhs)->get_lin(), false);
        else
            return lin_slv.new_gt(std::static_pointer_cast<const riddle::arith_item>(lhs)->get_lin(), std::static_pointer_cast<const riddle::arith_item>(rhs)->get_lin(), false, dynamic_cast<resolver &>(*get_current_resolver()).lin_cnsts);
    }
    bool solver::mk_gt(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept
    {
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            return lin_slv.new_gt(std::static_pointer_cast<const riddle::arith_item>(lhs)->get_lin(), std::static_pointer_cast<const riddle::arith_item>(rhs)->get_lin(), true);
        else
            return lin_slv.new_gt(std::static_pointer_cast<const riddle::arith_item>(lhs)->get_lin(), std::static_pointer_cast<const riddle::arith_item>(rhs)->get_lin(), true, dynamic_cast<resolver &>(*get_current_resolver()).lin_cnsts);
    }

    bool solver::mk_assign(riddle::enum_expr lhs, const utils::enum_val &val) noexcept
    {
        auto &c = ac_slv.new_assign(std::static_pointer_cast<const riddle::enum_item>(lhs)->get_var(), val);
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            ac_slv.add_constraint(c);
        else
            dynamic_cast<resolver &>(*get_current_resolver()).ac_cnsts.push_back(c);
        return true;
    }
    bool solver::mk_forbid(riddle::enum_expr lhs, const utils::enum_val &val) noexcept
    {
        auto &c = ac_slv.new_forbid(std::static_pointer_cast<const riddle::enum_item>(lhs)->get_var(), val);
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            ac_slv.add_constraint(c);
        else
            dynamic_cast<resolver &>(*get_current_resolver()).ac_cnsts.push_back(c);
        return true;
    }
    bool solver::mk_eq(riddle::enum_expr lhs, riddle::enum_expr rhs) noexcept
    {
        auto &c = ac_slv.new_equal(std::static_pointer_cast<const riddle::enum_item>(lhs)->get_var(), std::static_pointer_cast<const riddle::enum_item>(rhs)->get_var());
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            ac_slv.add_constraint(c);
        else
            dynamic_cast<resolver &>(*get_current_resolver()).ac_cnsts.push_back(c);
        return true;
    }
    bool solver::mk_neq(riddle::enum_expr lhs, riddle::enum_expr rhs) noexcept
    {
        auto &c = ac_slv.new_distinct(std::static_pointer_cast<const riddle::enum_item>(lhs)->get_var(), std::static_pointer_cast<const riddle::enum_item>(rhs)->get_var());
        if (!get_current_resolver() || sat_val(dynamic_cast<const resolver &>(*get_current_resolver()).get_rho()) == utils::True)
            ac_slv.add_constraint(c);
        else
            dynamic_cast<resolver &>(*get_current_resolver()).ac_cnsts.push_back(c);
        return true;
    }

    state_variable::state_variable(solver &slv) noexcept : riddle::state_variable(slv) {}
    std::shared_ptr<riddle::flaw> state_variable::new_peak(std::vector<riddle::atom_expr> &&atms) noexcept { return std::make_shared<sv_peak>(static_cast<solver &>(get_core()), std::move(atms)); }

    reusable_resource::reusable_resource(solver &slv) noexcept : riddle::reusable_resource(slv) {}
    std::shared_ptr<riddle::flaw> reusable_resource::new_peak(std::vector<riddle::atom_expr> &&atms) noexcept { return std::make_shared<rr_peak>(static_cast<solver &>(get_core()), std::move(atms)); }

    consumable_resource::consumable_resource(solver &slv) noexcept : riddle::consumable_resource(slv) {}

    std::shared_ptr<riddle::flaw> consumable_resource::new_overproduction(std::vector<riddle::atom_expr> &&prod_atms, std::vector<riddle::atom_expr> &&cons_atms) noexcept { return std::make_shared<cr_overproduction>(static_cast<solver &>(get_core()), std::move(prod_atms), std::move(cons_atms)); }
    std::shared_ptr<riddle::flaw> consumable_resource::new_overconsumption(std::vector<riddle::atom_expr> &&cons_atms, std::vector<riddle::atom_expr> &&prod_atms) noexcept { return std::make_shared<cr_overconsumption>(static_cast<solver &>(get_core()), std::move(cons_atms), std::move(prod_atms)); }
} // namespace ratio
