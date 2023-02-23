#include "solver.h"
#include "items.h"
#include "enum_flaw.h"
#include "disj_flaw.h"
#ifdef BUILD_LISTENERS
#include "solver_listener.h"
#endif
#include <cassert>

namespace ratio
{
    solver::solver() : theory(new semitone::sat_core()), lra_th(sat), ov_th(sat), idl_th(sat), rdl_th(sat) {}

    riddle::expr solver::new_bool() { return new bool_item(get_bool_type(), semitone::lit(sat->new_var())); }
    riddle::expr solver::new_bool(bool value) { return new bool_item(get_bool_type(), value ? semitone::TRUE_lit : semitone::FALSE_lit); }

    riddle::expr solver::new_int() { return new arith_item(get_int_type(), semitone::lin(lra_th.new_var(), utils::rational::ONE)); }
    riddle::expr solver::new_int(utils::I value) { return new arith_item(get_int_type(), semitone::lin(utils::rational(value))); }

    riddle::expr solver::new_real() { return new arith_item(get_real_type(), semitone::lin(lra_th.new_var(), utils::rational::ONE)); }
    riddle::expr solver::new_real(utils::rational value) { return new arith_item(get_real_type(), semitone::lin(value)); }

    riddle::expr solver::new_time_point() { return new arith_item(get_time_type(), semitone::lin(lra_th.new_var(), utils::rational::ONE)); }
    riddle::expr solver::new_time_point(utils::rational value) { return new arith_item(get_time_type(), semitone::lin(value)); }

    riddle::expr solver::new_string() { return new string_item(get_string_type()); }
    riddle::expr solver::new_string(const std::string &value) { return new string_item(get_string_type(), value); }

    riddle::expr solver::new_item(riddle::complex_type &tp) { return new riddle::complex_item(tp); }

    riddle::expr solver::new_enum(riddle::type &tp, const std::vector<riddle::expr> &xprs)
    {
        assert(tp != get_bool_type() && tp != get_int_type() && tp != get_real_type() && tp != get_time_type() && tp != get_string_type());
        switch (xprs.size())
        {
        case 0: // empty enum, we throw an exception..
            throw riddle::inconsistency_exception();
        case 1: // singleton enum, we return the expression..
            return xprs[0];
        default:
        {
            std::vector<utils::enum_val *> vals;
            vals.reserve(xprs.size());
            for (auto &xpr : xprs)
                if (auto vv = dynamic_cast<utils::enum_val *>(&*xpr))
                    vals.push_back(vv);
                else
                    throw std::runtime_error("enum values must be var values");

            // we create a new enum expression..
            // notice that we do not enforce the exct_one constraint!
            auto enum_expr = new enum_item(tp, ov_th.new_var(vals, false));
            new_flaw(new enum_flaw(*this, get_cause(), *enum_expr));
            return enum_expr;
        }
        }
    }

    riddle::expr solver::get_enum(riddle::expr &xpr, const std::string &name)
    {
        if (auto enum_expr = dynamic_cast<enum_item *>(&*xpr))
        { // we retrieve the domain of the enum variable..
            auto vs = ov_th.value(enum_expr->get_var());
            assert(vs.size() > 1);
            std::unordered_map<riddle::item *, std::vector<semitone::lit>> val_vars;
            for (auto &v : vs)
                val_vars[&*static_cast<riddle::complex_item *>(v)->get(name)].push_back(ov_th.allows(enum_expr->get_var(), *v));
            std::vector<semitone::lit> c_vars;
            std::vector<riddle::item *> c_vals;
            for (const auto &val : val_vars)
            {
                const auto c_var = sat->new_disj(val.second);
                c_vars.push_back(c_var);
                c_vals.push_back(val.first);
                for (const auto &val_not : val_vars)
                    if (val != val_not)
                        for (const auto &v : val_not.second)
                        {
                            [[maybe_unused]] bool nc = sat->new_clause({!c_var, !v});
                            assert(nc);
                        }
            }

            if (xpr->get_type() == get_bool_type())
            { // we have an enum of booleans..
                // we create a new boolean variable..
                auto b_xpr = new_bool();
                // .. and we add the constraints..
                [[maybe_unused]] bool nc;
                for (size_t i = 0; i < c_vars.size(); ++i)
                {
                    nc = sat->new_clause({!c_vars[i], sat->new_eq(static_cast<bool_item &>(*c_vals[i]).get_lit(), static_cast<bool_item &>(*b_xpr).get_lit())});
                    assert(nc);
                }
                return b_xpr;
            }
            else if (xpr->get_type() == get_int_type() || xpr->get_type() == get_real_type())
            {
                auto min = utils::inf_rational(utils::rational::POSITIVE_INFINITY);
                auto max = utils::inf_rational(utils::rational::NEGATIVE_INFINITY);
                // we compute the min and max values..
                for (const auto &val : c_vals)
                {
                    const auto [lb, ub] = lra_th.bounds(static_cast<arith_item *>(val)->get_lin());
                    min = std::min(min, lb);
                    max = std::max(max, ub);
                }
                if (min == max) // we have a constant..
                    return xpr->get_type() == get_int_type() ? new_int(min.get_rational().numerator()) : new_real(min.get_rational());
                else
                { // we create a new arithmetic variable..
                    auto arith_xpr = xpr->get_type() == get_int_type() ? new_int() : new_real();
                    // .. and we add the constraints..
                    [[maybe_unused]] bool nc;
                    for (size_t i = 0; i < c_vars.size(); ++i)
                    {
                        nc = sat->new_clause({!c_vars[i], lra_th.new_eq(static_cast<arith_item &>(*arith_xpr).get_lin(), static_cast<arith_item *>(c_vals[i])->get_lin())});
                        assert(nc);
                    }
                    // we try to impose some bounds which might help propagation..
                    if (min.get_infinitesimal() == utils::rational::ZERO && min.get_rational() > utils::rational::NEGATIVE_INFINITY)
                    {
                        nc = sat->new_clause({lra_th.new_geq(static_cast<arith_item &>(*arith_xpr).get_lin(), semitone::lin(min.get_rational()))});
                        assert(nc);
                    }
                    if (max.get_infinitesimal() == utils::rational::ZERO && max.get_rational() < utils::rational::POSITIVE_INFINITY)
                    {
                        nc = sat->new_clause({lra_th.new_leq(static_cast<arith_item &>(*arith_xpr).get_lin(), semitone::lin(max.get_rational()))});
                        assert(nc);
                    }
                    return arith_xpr;
                }
            }
            else if (xpr->get_type() == get_time_type())
            {
                auto min = utils::inf_rational(utils::rational::POSITIVE_INFINITY);
                auto max = utils::inf_rational(utils::rational::NEGATIVE_INFINITY);
                // we compute the min and max values..
                for (const auto &val : c_vals)
                {
                    const auto [lb, ub] = rdl_th.bounds(static_cast<arith_item *>(val)->get_lin());
                    min = std::min(min, lb);
                    max = std::max(max, ub);
                }
                if (min == max) // we have a constant..
                    return new_time_point(min.get_rational());
                else
                { // we create a new temporal variable..
                    auto tp_xpr = new_time_point();
                    // .. and we add the constraints..
                    [[maybe_unused]] bool nc;
                    for (size_t i = 0; i < c_vars.size(); ++i)
                    {
                        nc = sat->new_clause({!c_vars[i], rdl_th.new_eq(static_cast<arith_item &>(*tp_xpr).get_lin(), static_cast<arith_item *>(c_vals[i])->get_lin())});
                        assert(nc);
                    }
                    // we try to impose some bounds which might help propagation..
                    if (min.get_infinitesimal() == utils::rational::ZERO && min.get_rational() > utils::rational::NEGATIVE_INFINITY)
                    {
                        nc = sat->new_clause({rdl_th.new_geq(static_cast<arith_item &>(*tp_xpr).get_lin(), semitone::lin(min.get_rational()))});
                        assert(nc);
                    }
                    if (max.get_infinitesimal() == utils::rational::ZERO && max.get_rational() < utils::rational::POSITIVE_INFINITY)
                    {
                        nc = sat->new_clause({rdl_th.new_leq(static_cast<arith_item &>(*tp_xpr).get_lin(), semitone::lin(max.get_rational()))});
                        assert(nc);
                    }
                    return tp_xpr;
                }
            }
            else
            {
                std::vector<utils::enum_val *> e_vals;
                e_vals.reserve(c_vals.size());
                for (const auto &val : c_vals)
                    e_vals.push_back(dynamic_cast<utils::enum_val *>(val));
                return new enum_item(static_cast<riddle::complex_type &>(xpr->get_type()).get_field(name).get_type(), ov_th.new_var(c_vars, e_vals));
            }
        }
        else
            throw std::runtime_error("the expression must be an enum");
    }

    riddle::expr solver::add(const std::vector<riddle::expr> &xprs)
    {
        if (xprs.empty())
            return new_int(0);
        else if (xprs.size() == 1)
            return xprs[0];
        else
        {
            semitone::lin l;
            for (const auto &xpr : xprs)
                if (xpr->get_type() == get_int_type() || xpr->get_type() == get_real_type())
                    l += static_cast<arith_item &>(*xpr).get_lin();
                else
                    throw std::runtime_error("the expression must be an integer or a real");
            return new arith_item(get_type(xprs), l);
        }
    }

    riddle::expr solver::sub(const std::vector<riddle::expr> &xprs)
    {
        if (xprs.empty())
            throw std::runtime_error("the expression must be an integer or a real");
        else if (xprs.size() == 1)
            return new arith_item(get_type(xprs), -static_cast<arith_item &>(*xprs[0]).get_lin());
        else
        {
            semitone::lin l = static_cast<arith_item &>(*xprs[0]).get_lin();
            for (size_t i = 1; i < xprs.size(); ++i)
                if (xprs[i]->get_type() == get_int_type() || xprs[i]->get_type() == get_real_type())
                    l -= static_cast<arith_item &>(*xprs[i]).get_lin();
                else
                    throw std::runtime_error("the expression must be an integer or a real");
            return new arith_item(get_type(xprs), l);
        }
    }

    riddle::expr solver::mul(const std::vector<riddle::expr> &xprs)
    {
        if (xprs.empty())
            return new_int(1);
        else if (xprs.size() == 1)
            return xprs[0];
        else
        {
            if (auto var_it = std::find_if(xprs.cbegin(), xprs.cend(), [this](const auto &ae)
                                           { return is_constant(ae); });
                var_it != xprs.cend())
            {
                auto c_xpr = *var_it;
                semitone::lin l = static_cast<arith_item &>(*c_xpr).get_lin();
                for (const auto &xpr : xprs)
                    if (xpr != c_xpr)
                    {
                        assert(is_constant(xpr) && "non-linear expression..");
                        assert(lra_th.value(static_cast<arith_item &>(*xpr).get_lin()).get_infinitesimal() == utils::rational::ZERO);
                        l *= lra_th.value(static_cast<arith_item &>(*xpr).get_lin()).get_rational();
                    }
                return new arith_item(get_type(xprs), l);
            }
            else
            {
                semitone::lin l = static_cast<arith_item &>(**xprs.cbegin()).get_lin();
                for (auto xpr_it = ++xprs.cbegin(); xpr_it != xprs.cend(); ++xpr_it)
                {
                    assert(lra_th.value(static_cast<arith_item &>(**xpr_it).get_lin()).get_infinitesimal() == utils::rational::ZERO);
                    l *= lra_th.value(static_cast<arith_item &>(**xpr_it).get_lin()).get_rational();
                }
                return new arith_item(get_type(xprs), l);
            }
        }
    }

    riddle::expr solver::div(const std::vector<riddle::expr> &xprs)
    {
        if (xprs.empty())
            throw std::runtime_error("the expression must be an integer or a real");
        else if (xprs.size() == 1)
        {
            assert(is_constant(xprs[0]));
            assert(lra_th.value(static_cast<arith_item &>(*xprs[0]).get_lin()).get_infinitesimal() == utils::rational::ZERO);
            semitone::lin l(utils::rational::ONE);
            l /= static_cast<arith_item &>(*xprs[0]).get_lin().known_term;
            return new arith_item(get_type(xprs), l);
        }
        else
        {
            semitone::lin l = static_cast<arith_item &>(*xprs[0]).get_lin();
            for (size_t i = 1; i < xprs.size(); ++i)
                if (xprs[i]->get_type() == get_int_type() || xprs[i]->get_type() == get_real_type())
                {
                    assert(is_constant(xprs[i]));
                    assert(lra_th.value(static_cast<arith_item &>(*xprs[i]).get_lin()).get_infinitesimal() == utils::rational::ZERO);
                    l /= lra_th.value(static_cast<arith_item &>(*xprs[i]).get_lin()).get_rational();
                }
                else if (xprs[i]->get_type() == get_time_type())
                {
                    assert(is_constant(xprs[i]));
                    assert(rdl_th.bounds(static_cast<arith_item &>(*xprs[i]).get_lin()).first.get_infinitesimal() == utils::rational::ZERO);
                    l /= rdl_th.bounds(static_cast<arith_item &>(*xprs[i]).get_lin()).first.get_rational();
                }
                else
                    throw std::runtime_error("the expression must be an integer or a real");
            return new arith_item(get_type(xprs), l);
        }
    }

    riddle::expr solver::minus(const riddle::expr &xpr)
    {
        if (xpr->get_type() == get_int_type() || xpr->get_type() == get_real_type() || xpr->get_type() == get_time_type())
            return new arith_item(xpr->get_type(), -static_cast<arith_item &>(*xpr).get_lin());
        else
            throw std::runtime_error("the expression must be an integer or a real");
    }

    riddle::expr solver::lt(const riddle::expr &lhs, const riddle::expr &rhs)
    {
        if ((lhs->get_type() == get_int_type() || lhs->get_type() == get_real_type()) && (rhs->get_type() == get_int_type() || rhs->get_type() == get_real_type()))
            return new bool_item(get_bool_type(), lra_th.new_lt(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
        else if (lhs->get_type() == get_time_type() && rhs->get_type() == get_time_type())
            return new bool_item(get_bool_type(), rdl_th.new_lt(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
        else
            throw std::runtime_error("the expression must be an integer or a real");
    }

    riddle::expr solver::leq(const riddle::expr &lhs, const riddle::expr &rhs)
    {
        if ((lhs->get_type() == get_int_type() || lhs->get_type() == get_real_type()) && (rhs->get_type() == get_int_type() || rhs->get_type() == get_real_type()))
            return new bool_item(get_bool_type(), lra_th.new_leq(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
        else if (lhs->get_type() == get_time_type() && rhs->get_type() == get_time_type())
            return new bool_item(get_bool_type(), rdl_th.new_leq(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
        else
            throw std::runtime_error("the expression must be an integer or a real");
    }

    riddle::expr solver::eq(const riddle::expr &lhs, const riddle::expr &rhs)
    {
        if (lhs == rhs) // the two items are the same item..
            return new bool_item(get_bool_type(), semitone::TRUE_lit);
        else if ((lhs->get_type() == get_int_type() || lhs->get_type() == get_real_type()) && lhs->get_type() == rhs->get_type())
            return new bool_item(get_bool_type(), lra_th.new_eq(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
        else if (lhs->get_type() == get_time_type() && lhs->get_type() == rhs->get_type())
            return new bool_item(get_bool_type(), rdl_th.new_eq(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
        else if (lhs->get_type() == get_bool_type() && lhs->get_type() == rhs->get_type())
            return new bool_item(get_bool_type(), sat->new_eq(static_cast<bool_item &>(*lhs).get_lit(), static_cast<bool_item &>(*rhs).get_lit()));
        else if (lhs->get_type() == get_string_type() && lhs->get_type() == rhs->get_type())
            return new bool_item(get_bool_type(), static_cast<string_item &>(*lhs).get_string() == static_cast<string_item &>(*rhs).get_string() ? semitone::TRUE_lit : semitone::FALSE_lit);
        else if (auto lee = dynamic_cast<enum_item *>(&*lhs))
        {                                                    // we are comparing an enum with something else..
            if (auto ree = dynamic_cast<enum_item *>(&*rhs)) // we are comparing enums..
                return new bool_item(get_bool_type(), ov_th.new_eq(lee->get_var(), ree->get_var()));
            else // we are comparing an enum with a singleton..
                return new bool_item(get_bool_type(), ov_th.allows(lee->get_var(), dynamic_cast<utils::enum_val &>(*rhs)));
        }
        else if (dynamic_cast<enum_item *>(&*rhs))
            return eq(rhs, lhs); // we swap, for simplifying code..
        else if (lhs->get_type() == rhs->get_type())
        { // we are comparing complex items..
            auto leci = dynamic_cast<riddle::complex_item *>(&*lhs);
            if (!leci)
                throw std::runtime_error("the expression must be a complex item");
            auto reci = dynamic_cast<riddle::complex_item *>(&*rhs);
            if (!reci)
                throw std::runtime_error("the expression must be a complex item");

            std::vector<semitone::lit> eqs;
            if (auto p = dynamic_cast<riddle::predicate *>(&lhs->get_type()))
            {
                std::queue<riddle::predicate *> q;
                q.push(p);
                while (!q.empty())
                {
                    for (const auto &[f_name, f] : q.front()->get_fields())
                        if (!f->is_synthetic())
                        {
                            auto c_eq = eq(leci->get(f_name), reci->get(f_name));
                            if (bool_value(c_eq) == utils::False)
                                return new bool_item(get_bool_type(), semitone::FALSE_lit);
                            eqs.push_back(static_cast<bool_item &>(*c_eq).get_lit());
                        }
                    for (const auto &stp : q.front()->get_parents())
                        q.push(&stp.get());
                    q.pop();
                }
            }
            else if (auto ct = dynamic_cast<riddle::complex_type *>(&lhs->get_type()))
            {
                std::queue<riddle::complex_type *> q;
                q.push(ct);
                while (!q.empty())
                {
                    for (const auto &[f_name, f] : q.front()->get_fields())
                        if (!f->is_synthetic())
                        {
                            auto c_eq = eq(leci->get(f_name), reci->get(f_name));
                            if (bool_value(c_eq) == utils::False)
                                return new bool_item(get_bool_type(), semitone::FALSE_lit);
                            eqs.push_back(static_cast<bool_item &>(*c_eq).get_lit());
                        }
                    for (const auto &stp : q.front()->get_parents())
                        q.push(&stp.get());
                    q.pop();
                }
            }
            switch (eqs.size())
            {
            case 0:
                return new bool_item(get_bool_type(), semitone::FALSE_lit);
            case 1:
                return new bool_item(get_bool_type(), eqs[0]);
            default:
                return new bool_item(get_bool_type(), sat->new_conj(eqs));
            }
        }
        else
            return new bool_item(get_bool_type(), semitone::FALSE_lit);
    }

    riddle::expr solver::geq(const riddle::expr &lhs, const riddle::expr &rhs)
    {
        if ((lhs->get_type() == get_int_type() || lhs->get_type() == get_real_type()) && (rhs->get_type() == get_int_type() || rhs->get_type() == get_real_type()))
            return new bool_item(get_bool_type(), lra_th.new_geq(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
        else if (lhs->get_type() == get_time_type() && rhs->get_type() == get_time_type())
            return new bool_item(get_bool_type(), rdl_th.new_geq(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
        else
            throw std::runtime_error("the expression must be an integer or a real");
    }

    riddle::expr solver::gt(const riddle::expr &lhs, const riddle::expr &rhs)
    {
        if ((lhs->get_type() == get_int_type() || lhs->get_type() == get_real_type()) && (rhs->get_type() == get_int_type() || rhs->get_type() == get_real_type()))
            return new bool_item(get_bool_type(), lra_th.new_gt(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
        else if (lhs->get_type() == get_time_type() && rhs->get_type() == get_time_type())
            return new bool_item(get_bool_type(), rdl_th.new_gt(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
        else
            throw std::runtime_error("the expression must be an integer or a real");
    }

    riddle::expr solver::conj(const std::vector<riddle::expr> &xprs)
    {
        std::vector<semitone::lit> lits;
        for (const auto &xpr : xprs)
            lits.push_back(static_cast<bool_item &>(*xpr).get_lit());
        return new bool_item(get_bool_type(), sat->new_conj(lits));
    }

    riddle::expr solver::disj(const std::vector<riddle::expr> &xprs)
    {
        std::vector<semitone::lit> lits;
        for (const auto &xpr : xprs)
            lits.push_back(static_cast<bool_item &>(*xpr).get_lit());
        if (lits.size() > 1)
            new_flaw(new disj_flaw(*this, get_cause(), lits));
        return new bool_item(get_bool_type(), sat->new_disj(lits));
    }

    riddle::expr solver::exct_one(const std::vector<riddle::expr> &xprs)
    {
        std::vector<semitone::lit> lits;
        for (const auto &xpr : xprs)
            lits.push_back(static_cast<bool_item &>(*xpr).get_lit());
        if (lits.size() > 1)
            new_flaw(new disj_flaw(*this, get_cause(), lits));
        return new bool_item(get_bool_type(), sat->new_exct_one(lits));
    }

    riddle::expr solver::negate(const riddle::expr &xpr) { return new bool_item(get_bool_type(), !static_cast<bool_item &>(*xpr).get_lit()); }

    void solver::assert_fact(const riddle::expr &xpr)
    { // the expression must be a boolean..
        if (xpr->get_type() == get_bool_type())
        {
            if (!sat->new_clause({!ni, static_cast<ratio::bool_item &>(*xpr).get_lit()}))
                throw riddle::unsolvable_exception(); // the problem is unsolvable
        }
        else
            throw std::runtime_error("the expression must be a boolean");
    }

    void solver::new_disjunction(std::vector<riddle::conjunction_ptr> xprs)
    {
    }

#ifdef BUILD_LISTENERS
    void solver::fire_new_flaw(const flaw &f) const
    {
        for (auto &l : listeners)
            l->new_flaw(f);
    }
    void solver::fire_flaw_state_changed(const flaw &f) const
    {
        for (auto &l : listeners)
            l->flaw_state_changed(f);
    }
    void solver::fire_flaw_cost_changed(const flaw &f) const
    {
        for (auto &l : listeners)
            l->flaw_cost_changed(f);
    }
    void solver::fire_current_flaw(const flaw &f) const
    {
        for (auto &l : listeners)
            l->current_flaw(f);
    }
    void solver::fire_new_resolver(const resolver &r) const
    {
        for (auto &l : listeners)
            l->new_resolver(r);
    }
    void solver::fire_resolver_state_changed(const resolver &r) const
    {
        for (auto &l : listeners)
            l->resolver_state_changed(r);
    }
    void solver::fire_current_resolver(const resolver &r) const
    {
        for (auto &l : listeners)
            l->current_resolver(r);
    }
    void solver::fire_causal_link_added(const flaw &f, const resolver &r) const
    {
        for (auto &l : listeners)
            l->causal_link_added(f, r);
    }
#endif

    json::json value(const riddle::item &itm) noexcept
    {
        const solver &slv = static_cast<const solver &>(itm.get_type().get_scope().get_core());
        if (itm.get_type() == slv.get_bool_type())
        {
            auto val = static_cast<const bool_item &>(itm).get_lit();
            json::json j_val;
            j_val["lit"] = (sign(val) ? "b" : "!b") + std::to_string(variable(val));
            switch (slv.get_sat_core().value(val))
            {
            case utils::True:
                j_val["val"] = "True";
                break;
            case utils::False:
                j_val["val"] = "False";
                break;
            case utils::Undefined:
                j_val["val"] = "Undefined";
                break;
            }
            return j_val;
        }
        else if (itm.get_type() == slv.get_int_type() || itm.get_type() == slv.get_real_type())
        {
            auto lin = static_cast<const arith_item &>(itm).get_lin();
            const auto [lb, ub] = slv.get_lra_theory().bounds(lin);
            const auto val = slv.get_lra_theory().value(lin);

            json::json j_val = to_json(val);
            j_val["lin"] = to_string(lin);
            if (!is_negative_infinite(lb))
                j_val["lb"] = to_json(lb);
            if (!is_positive_infinite(ub))
                j_val["ub"] = to_json(ub);
            return j_val;
        }
        else if (itm.get_type() == slv.get_time_type())
        {
            auto lin = static_cast<const arith_item &>(itm).get_lin();
            const auto [lb, ub] = slv.get_rdl_theory().bounds(lin);

            json::json j_val = to_json(lb);
            j_val["lin"] = to_string(lin);
            if (!is_negative_infinite(lb))
                j_val["lb"] = to_json(lb);
            if (!is_positive_infinite(ub))
                j_val["ub"] = to_json(ub);
            return j_val;
        }
        else if (itm.get_type() == slv.get_string_type())
            return static_cast<const string_item &>(itm).get_string();
        else if (auto ev = dynamic_cast<const enum_item *>(&itm))
        {
            json::json j_val;
            j_val["var"] = std::to_string(ev->get_var());
            json::json vals;
            for (const auto &v : slv.get_ov_theory().value(ev->get_var()))
                vals.push_back(get_id(static_cast<riddle::complex_item &>(*v)));
            j_val["vals"] = std::move(vals);
            return j_val;
        }
        else
            return get_id(itm);
    }
} // namespace ratio
