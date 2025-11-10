#include "solver.hpp"
#include "flaws.hpp"
#include "logging.hpp"
#include <stack>
#include <cassert>

#ifdef ORATIO_ENABLE_LISTENERS
#define STATE_CHANGED() state_changed()
#define FLAW_STATE_CHANGED(f) flaw_state_changed(f)
#define FLAW_COST_CHANGED(f) flaw_cost_changed(f)
#define FLAW_POSITION_CHANGED(f) flaw_position_changed(f)
#define RESOLVER_STATE_CHANGED(r) resolver_state_changed(r)
#define NEW_CAUSAL_LINK(f, r) causal_link_added(f, r)
#define CURRENT_FLAW(f) current_flaw(f)
#define CURRENT_RESOLVER(r) current_resolver(r)
#else
#define STATE_CHANGED()
#define FLAW_STATE_CHANGED(f)
#define FLAW_COST_CHANGED(f)
#define FLAW_POSITION_CHANGED(f)
#define RESOLVER_STATE_CHANGED(r)
#define NEW_CAUSAL_LINK(f, r)
#define CURRENT_FLAW(f)
#define CURRENT_RESOLVER(r)
#endif

namespace ratio
{
    solver::solver(std::string_view name) noexcept : riddle::core(name) {}

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

    utils::inf_rational solver::arith_value(const riddle::arith_term &expr) const noexcept { return lin_slv.val(static_cast<const riddle::arith_item &>(expr).get_lin()); }

    riddle::string_expr solver::new_string() { return std::make_shared<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ""); }
    riddle::string_expr solver::new_string(std::string &&value) { return std::make_shared<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), std::move(value)); }
    std::string solver::string_value(const riddle::string_term &expr) const noexcept { return static_cast<const riddle::string_item &>(expr).get_string(); }

    riddle::enum_expr solver::new_enum(riddle::component_type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values)
    {
        assert(!values.empty());
        std::vector<std::reference_wrapper<resolver>> causes;
        if (c_res)
            causes.push_back(c_res.value());

        std::vector<utils::lit> lits;
        if (values.size() == 1)
        {
            auto ev = ac_slv.new_var(values);
            return std::make_shared<riddle::enum_item>(tp, std::move(values), ev);
        }
        else
        {
            auto ev = ac_slv.new_var(values);
            // .. and create a new enum flaw to manage the variable..
            auto &ef = new_flaw<enum_flaw>(*this, std::move(causes), std::make_shared<riddle::enum_item>(tp, std::move(values), ev));
            return ef.get_var();
        }
    }
    std::vector<std::reference_wrapper<utils::enum_val>> solver::enum_value(const riddle::enum_term &expr) const noexcept { return ac_slv.domain(static_cast<const riddle::enum_item &>(expr).get_var()); }

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
        for (const riddle::arith_expr &xpr : xprs)
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
        for (const riddle::arith_expr &xpr : xprs)
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
            std::vector<std::reference_wrapper<resolver>> causes;
            if (c_res)
                causes.push_back(c_res.value());

            std::vector<utils::lit> clause;
            clause.reserve(exprs.size());
            for (const riddle::bool_expr &expr : exprs)
                clause.push_back(static_cast<const riddle::bool_item &>(*expr).get_lit());

            auto ac_cnstr = ac_slv.new_clause(std::move(clause));
            if (c_res) // if there is a current resolver, add the expression to it..
                c_res.value().get().ac_cnsts.push_back(ac_cnstr);
            else
                ac_slv.add_constraint(ac_cnstr);
            new_flaw<clause_flaw>(*this, std::move(causes), std::move(exprs));
        }
    }
    void solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<std::reference_wrapper<resolver>> causes;
        if (c_res)
            causes.push_back(c_res.value());

        new_flaw<disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
    }

    void solver::solve()
    {
        if (!lin_slv.check() || !ac_slv.propagate())
            throw std::runtime_error("Unsatisfiable constraints");
        auto it = std::find_if(active_flaws.begin(), active_flaws.end(), [](flaw *f)
                               { return utils::is_positive_infinite(f->get_estimated_cost()); });
        while (it != active_flaws.end())
        {
            auto &f = **it;
            CURRENT_FLAW(f);
            assert(f.get_state() == utils::True && "Active flaw found to be inactive.");
            if (f.is_expanded())
            {
            }
            else
            {
                LOG_TRACE("Computing resolvers for " << f.to_json());
                f.compute_resolvers();
                f.expanded = true;
                assert(std::none_of(f.get_resolvers().begin(), f.get_resolvers().end(), [&](resolver &r)
                                    { return r.get_state() == utils::False; }) &&
                       "Computed resolver found to be inactive.");
                for (resolver &r : f.get_resolvers())
                {
                    CURRENT_RESOLVER(r);
                    LOG_TRACE("Applying resolver " << r.to_json());
                    assert(r.get_state() && "Computed resolver found to be inactive.");
                    r.apply();
                }
            }
            compute_flaw_cost(f);
            active_flaws.erase(it);
            it = std::find_if(active_flaws.begin(), active_flaws.end(), [](flaw *f)
                              { return utils::is_positive_infinite(f->get_estimated_cost()); });
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
        else
            throw std::runtime_error("Matching not supported for this term type");
    }

    riddle::atom_expr solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args)
    {
        std::vector<std::reference_wrapper<resolver>> causes;
        if (c_res)
            causes.push_back(c_res.value());

        auto &af = new_flaw<atom_flaw>(*this, std::move(causes), is_fact, pred, std::move(args), ac_slv.new_sat());
        return af.get_atom();
    }
    riddle::atom_state solver::get_atom_state(const riddle::atom_term &atm) const noexcept
    {
        switch (ac_slv.sat_val(static_cast<const atom &>(atm).get_sigma()))
        {
        case utils::True:
            return riddle::active;
        case utils::False:
            return riddle::unified;
        default:
            return riddle::inactive;
        }
    }

    bool solver::execute(const riddle::bool_expr &expr) noexcept
    {
        if (auto n_xpr = dynamic_cast<riddle::bool_not *>(expr.get()))
        {
            if (auto b_xpr = dynamic_cast<riddle::bool_item *>(n_xpr->get_arg().get()))
            {
                auto a_cnstr = ac_slv.new_assign(utils::variable(b_xpr->get_lit()), utils::sign(b_xpr->get_lit()) ? arc_consistency::solver::False : arc_consistency::solver::True);
                ac_slv.add_constraint(a_cnstr);
                if (c_res)
                    c_res.value().get().ac_cnsts.push_back(a_cnstr);
                return true;
            }
            else if (auto lt_xpr = dynamic_cast<riddle::lt_term *>(n_xpr->get_arg().get()))
                return lin_slv.new_gt(static_cast<riddle::arith_item *>(lt_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(lt_xpr->get_rhs().get())->get_lin(), false, c_res ? c_res.value().get().cnst : nullptr);
            else if (auto le_xpr = dynamic_cast<riddle::le_term *>(n_xpr->get_arg().get()))
                return lin_slv.new_gt(static_cast<riddle::arith_item *>(le_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(le_xpr->get_rhs().get())->get_lin(), true, c_res ? c_res.value().get().cnst : nullptr);
            else if (auto eq_xpr = dynamic_cast<riddle::eq_term *>(n_xpr->get_arg().get()))
            {
                if (&*eq_xpr->get_lhs() == &*eq_xpr->get_rhs()) // the terms are the same, so they are equal..
                    return false;
                else if (&eq_xpr->get_lhs()->get_type() != &eq_xpr->get_lhs()->get_type()) // the types are different, so the constraint is always false..
                    return true;
                else if (auto lhs_xpr = std::dynamic_pointer_cast<riddle::arith_item>(eq_xpr->get_lhs())) // we are dealing with an arithmetic constraint..
                {
                    new_clause({std::make_shared<riddle::lt_term>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), lhs_xpr, std::static_pointer_cast<riddle::arith_item>(eq_xpr->get_rhs())), std::make_shared<riddle::gt_term>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), lhs_xpr, std::static_pointer_cast<riddle::arith_item>(eq_xpr->get_rhs()))});
                    return true;
                }
                else if (auto lhs_sxpr = dynamic_cast<riddle::string_item *>(eq_xpr->get_lhs().get())) // we are dealing with a string constraint..
                    return lhs_sxpr->get_string() != static_cast<riddle::string_item &>(*eq_xpr->get_rhs()).get_string();
                else if (auto lhs_bxpr = dynamic_cast<riddle::bool_item *>(eq_xpr->get_lhs().get())) // we are dealing with a boolean constraint..
                {
                    auto neq_cnstr = ac_slv.new_distinct(utils::variable(lhs_bxpr->get_lit()), utils::variable(static_cast<riddle::bool_item &>(*eq_xpr->get_rhs()).get_lit()));
                    ac_slv.add_constraint(neq_cnstr);
                    if (c_res) // if there is a current resolver, add the expression to it..
                        c_res.value().get().ac_cnsts.push_back(neq_cnstr);
                    return true;
                }
                else if (auto lhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_lhs().get())) // we are dealing with an enum constraint..
                {
                    if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                    { // both sides are enum items..
                        auto neq_cnstr = ac_slv.new_distinct(lhs_enum_xpr->get_var(), rhs_enum_xpr->get_var());
                        ac_slv.add_constraint(neq_cnstr);
                        if (c_res) // if there is a current resolver, add the expression to it..
                            c_res.value().get().ac_cnsts.push_back(neq_cnstr);
                        return true;
                    }
                    else
                    {
                        auto neq_cnstr = ac_slv.new_forbid(lhs_enum_xpr->get_var(), *eq_xpr->get_rhs());
                        ac_slv.add_constraint(neq_cnstr);
                        if (c_res) // if there is a current resolver, add the expression to it..
                            c_res.value().get().ac_cnsts.push_back(neq_cnstr);
                        return true;
                    }
                }
                else if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                {
                    auto neq_cnstr = ac_slv.new_forbid(rhs_enum_xpr->get_var(), *eq_xpr->get_lhs());
                    ac_slv.add_constraint(neq_cnstr);
                    if (c_res) // if there is a current resolver, add the expression to it..
                        c_res.value().get().ac_cnsts.push_back(neq_cnstr);
                    return true;
                }
                else if (auto lhs_atm = dynamic_cast<riddle::atom_term *>(eq_xpr->get_lhs().get()))
                {
                    auto rhs_atm = static_cast<riddle::atom_term *>(eq_xpr->get_rhs().get());
                    std::vector<riddle::bool_expr> clause_exprs;
                    std::queue<riddle::predicate *> q;
                    q.push(static_cast<riddle::predicate *>(&lhs_xpr->get_type()));
                    while (!q.empty())
                    {
                        for (const auto &[f_name, f] : q.front()->get_fields())
                            clause_exprs.push_back(std::make_shared<riddle::bool_not>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::make_shared<riddle::eq_term>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), lhs_atm->get(f_name), rhs_atm->get(f_name))));
                        for (const auto &pp : q.front()->get_parents())
                            q.push(&pp.get());
                        q.pop();
                    }
                    new_clause(std::move(clause_exprs));
                    return true;
                }
                else
                    return true;
            }
            else if (auto ge_xpr = dynamic_cast<riddle::ge_term *>(n_xpr->get_arg().get()))
                return lin_slv.new_lt(static_cast<riddle::arith_item *>(ge_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(ge_xpr->get_rhs().get())->get_lin(), false, c_res ? c_res.value().get().cnst : nullptr);
            else if (auto gt_xpr = dynamic_cast<riddle::gt_term *>(n_xpr->get_arg().get()))
                return lin_slv.new_lt(static_cast<riddle::arith_item *>(gt_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(gt_xpr->get_rhs().get())->get_lin(), true, c_res ? c_res.value().get().cnst : nullptr);
            else
                return false; // unknown expression inside negation..
        }
        else
        {
            if (auto b_xpr = dynamic_cast<riddle::bool_item *>(expr.get()))
            {
                auto a_cnstr = ac_slv.new_assign(utils::variable(b_xpr->get_lit()), utils::sign(b_xpr->get_lit()) ? arc_consistency::solver::True : arc_consistency::solver::False);
                ac_slv.add_constraint(a_cnstr);
                if (c_res)
                    c_res.value().get().ac_cnsts.push_back(a_cnstr);
                return true;
            }
            else if (auto lt_xpr = dynamic_cast<riddle::lt_term *>(expr.get()))
                return lin_slv.new_lt(static_cast<riddle::arith_item *>(lt_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(lt_xpr->get_rhs().get())->get_lin(), true, c_res ? c_res.value().get().cnst : nullptr);
            else if (auto le_xpr = dynamic_cast<riddle::le_term *>(expr.get()))
                return lin_slv.new_lt(static_cast<riddle::arith_item *>(le_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(le_xpr->get_rhs().get())->get_lin(), false, c_res ? c_res.value().get().cnst : nullptr);
            else if (auto eq_xpr = dynamic_cast<riddle::eq_term *>(expr.get()))
            {
                if (&*eq_xpr->get_lhs() == &*eq_xpr->get_rhs()) // the terms are the same, so they are equal..
                    return true;
                else if (&eq_xpr->get_lhs()->get_type() != &eq_xpr->get_lhs()->get_type()) // the types are different, so the constraint is always false..
                    return false;
                else if (auto lhs_xpr = dynamic_cast<riddle::arith_item *>(eq_xpr->get_lhs().get())) // we are dealing with an arithmetic constraint..
                    return lin_slv.new_eq(lhs_xpr->get_lin(), static_cast<riddle::arith_item *>(eq_xpr->get_rhs().get())->get_lin(), c_res ? c_res.value().get().cnst : nullptr);
                else if (auto lhs_sxpr = dynamic_cast<riddle::string_item *>(eq_xpr->get_lhs().get())) // we are dealing with a string constraint..
                    return lhs_sxpr->get_string() == static_cast<riddle::string_item &>(*eq_xpr->get_rhs()).get_string();
                else if (auto lhs_bxpr = dynamic_cast<riddle::bool_item *>(eq_xpr->get_lhs().get())) // we are dealing with a boolean constraint..
                {
                    auto eq_cnstr = ac_slv.new_equal(utils::variable(lhs_bxpr->get_lit()), utils::variable(static_cast<riddle::bool_item &>(*eq_xpr->get_rhs()).get_lit()));
                    ac_slv.add_constraint(eq_cnstr);
                    if (c_res) // if there is a current resolver, add the expression to it..
                        c_res.value().get().ac_cnsts.push_back(eq_cnstr);
                    return true;
                }
                else if (auto lhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_lhs().get())) // we are dealing with an enum constraint..
                {
                    if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                    { // both sides are enum items..
                        auto eq_cnstr = ac_slv.new_equal(lhs_enum_xpr->get_var(), rhs_enum_xpr->get_var());
                        ac_slv.add_constraint(eq_cnstr);
                        if (c_res) // if there is a current resolver, add the expression to it..
                            c_res.value().get().ac_cnsts.push_back(eq_cnstr);
                        return true;
                    }
                    else
                    {
                        auto eq_cnstr = ac_slv.new_assign(lhs_enum_xpr->get_var(), *eq_xpr->get_rhs());
                        ac_slv.add_constraint(eq_cnstr);
                        if (c_res) // if there is a current resolver, add the expression to it..
                            c_res.value().get().ac_cnsts.push_back(eq_cnstr);
                        return true;
                    }
                }
                else if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                {
                    auto eq_cnstr = ac_slv.new_assign(rhs_enum_xpr->get_var(), *eq_xpr->get_lhs());
                    ac_slv.add_constraint(eq_cnstr);
                    if (c_res) // if there is a current resolver, add the expression to it..
                        c_res.value().get().ac_cnsts.push_back(eq_cnstr);
                    return true;
                }
                else if (auto lhs_atm = dynamic_cast<riddle::atom_term *>(eq_xpr->get_lhs().get())) // we are dealing with an atom constraint..
                {
                    auto rhs_atm = static_cast<riddle::atom_term *>(eq_xpr->get_rhs().get());
                    std::queue<riddle::predicate *> q;
                    q.push(static_cast<riddle::predicate *>(&lhs_xpr->get_type()));
                    while (!q.empty())
                    {
                        for (const auto &[f_name, f] : q.front()->get_fields())
                            if (!execute(std::make_shared<riddle::eq_term>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), lhs_atm->get(f_name), rhs_atm->get(f_name))))
                                return false;
                        for (const auto &pp : q.front()->get_parents())
                            q.push(&pp.get());
                        q.pop();
                    }
                    return true;
                }
                else
                    return false;
            }
            else if (auto ge_xpr = dynamic_cast<riddle::ge_term *>(expr.get()))
                return lin_slv.new_gt(static_cast<riddle::arith_item *>(ge_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(ge_xpr->get_rhs().get())->get_lin(), false, c_res ? c_res.value().get().cnst : nullptr);
            else if (auto gt_xpr = dynamic_cast<riddle::gt_term *>(expr.get()))
                return lin_slv.new_gt(static_cast<riddle::arith_item *>(gt_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(gt_xpr->get_rhs().get())->get_lin(), true, c_res ? c_res.value().get().cnst : nullptr);
            else
                return false; // unsupported expression, just return false..
        }
    }

    void solver::compute_flaw_cost(flaw &f) noexcept
    {
        std::stack<std::pair<flaw *, std::unordered_set<flaw *>>> stk;
        stk.push({&f, {}}); // we push the flaw in the stack..

        while (!stk.empty())
        {
            auto c_f = stk.top();
            stk.pop();

            utils::rational c_cost = utils::rational::positive_infinite;
            if (c_f.first->get_state() != utils::False && c_f.second.insert(c_f.first).second) // we compute the cost of the flaw as the minimum of the costs of its resolvers..
                for (const auto &res : c_f.first->resolvers)
                    if (res.get().get_state() != utils::False)
                        c_cost = std::min(c_cost, res.get().get_estimated_cost());

            if (c_f.first->est_cost != c_cost) // we update the cost of the flaw..
            {
                c_f.first->est_cost = c_cost;
                FLAW_COST_CHANGED(*c_f.first);

                // we propagate the cost to the supported resolvers..
                for (auto &support : c_f.first->get_supports())
                    stk.push({&support.get().f, c_f.second}); // we push the supported flaw in the stack..
            }
        }
    }

    json::json solver::to_json() const
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
        if (c_flaw)
            j_graph["current_flaw"] = c_flaw.value().get().get_id();
        if (c_res)
            j_graph["current_resolver"] = c_res.value().get().get_id();

        return j_graph;
    }
} // namespace ratio
