#include "solver.h"
#include "init.h"
#include "bool_flaw.h"
#include "enum_flaw.h"
#include "disj_flaw.h"
#include "disjunction_flaw.h"
#include "atom_flaw.h"
#include "smart_type.h"
#include "agent.h"
#include "state_variable.h"
#include "reusable_resource.h"
#include "consumable_resource.h"
#if defined(H_MAX) || defined(H_ADD)
#include "h_1.h"
#define HEURISTIC new h_1(*this)
#endif
#ifdef BUILD_LISTENERS
#include "solver_listener.h"
#endif
#include <cassert>

namespace ratio
{
    solver::solver(const bool &i) : solver(HEURISTIC, i) {}
    solver::solver(graph_ptr g, const bool &i) : countable(true), theory(new semitone::sat_core()), lra_th(sat), ov_th(sat), idl_th(sat), rdl_th(sat), gr(std::move(g))
    {
        gr->reset_gamma();
        if (i) // we initializa the solver..
            init();
    }

    void solver::init()
    {
        // we read the init string..
        read(INIT_STRING);
        // we get the impulsive and interval predicates..
        imp_pred = &get_predicate(RATIO_IMPULSE);
        int_pred = &get_predicate(RATIO_INTERVAL);
        // we add some smart types..
        add_type(new agent(*this));
        add_type(new state_variable(*this));
        add_type(new reusable_resource(*this));
        add_type(new consumable_resource(*this));
    }

    void solver::read(const std::string &script)
    {
        // we read the script..
        core::read(script);
        // we reset the smart-types if some new smart-type has been added with the previous script..
        reset_smart_types();

        if (!sat->propagate())
            throw riddle::unsolvable_exception();
        FIRE_STATE_CHANGED();
    }
    void solver::read(const std::vector<std::string> &files)
    {
        // we read the files..
        core::read(files);
        // we reset the smart-types if some new smart-type has been added with the previous files..
        reset_smart_types();

        if (!sat->propagate())
            throw riddle::unsolvable_exception();
        FIRE_STATE_CHANGED();
    }

    riddle::expr solver::new_bool()
    {
        riddle::expr b_xpr = new bool_item(get_bool_type(), semitone::lit(sat->new_var()));
        new_flaw(new bool_flaw(*this, get_cause(), b_xpr));
        return b_xpr;
    }
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
                if (auto ev = dynamic_cast<utils::enum_val *>(xpr.operator->()))
                    vals.push_back(ev);
                else
                    throw std::runtime_error("invalid enum expression");

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
        if (auto enum_expr = dynamic_cast<enum_item *>(xpr.operator->()))
        { // we retrieve the domain of the enum variable..
            auto vs = ov_th.value(enum_expr->get_var());
            assert(vs.size() > 1);
            std::unordered_map<riddle::item *, std::vector<semitone::lit>> val_vars;
            for (auto &v : vs)
                val_vars[dynamic_cast<riddle::env *>(v)->get(name).operator->()].push_back(ov_th.allows(enum_expr->get_var(), *v));
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
        else if ((lhs->get_type() == get_int_type() || lhs->get_type() == get_real_type() || lhs->get_type() == get_time_type()) && (rhs->get_type() == get_int_type() || rhs->get_type() == get_real_type() || rhs->get_type() == get_time_type()))
        { // we are comparing two arithmetic expressions..
            if (get_type({lhs, rhs}) == get_time_type())
                return new bool_item(get_bool_type(), rdl_th.new_eq(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
            else
                return new bool_item(get_bool_type(), lra_th.new_eq(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin()));
        }
        else if (lhs->get_type() == get_bool_type() && lhs->get_type() == rhs->get_type())
            return new bool_item(get_bool_type(), sat->new_eq(static_cast<bool_item &>(*lhs).get_lit(), static_cast<bool_item &>(*rhs).get_lit()));
        else if (lhs->get_type() == get_string_type() && lhs->get_type() == rhs->get_type())
            return new bool_item(get_bool_type(), static_cast<string_item &>(*lhs).get_string() == static_cast<string_item &>(*rhs).get_string() ? semitone::TRUE_lit : semitone::FALSE_lit);
        else if (auto lee = dynamic_cast<enum_item *>(lhs.operator->()))
        {                                                               // we are comparing an enum with something else..
            if (auto ree = dynamic_cast<enum_item *>(rhs.operator->())) // we are comparing enums..
                return new bool_item(get_bool_type(), ov_th.new_eq(lee->get_var(), ree->get_var()));
            else // we are comparing an enum with a singleton..
                return new bool_item(get_bool_type(), ov_th.allows(lee->get_var(), dynamic_cast<utils::enum_val &>(*rhs)));
        }
        else if (dynamic_cast<enum_item *>(rhs.operator->()))
            return eq(rhs, lhs); // we swap, for simplifying code..
        else if (lhs->get_type() == rhs->get_type())
        { // we are comparing complex items..
            auto leci = dynamic_cast<riddle::complex_item *>(lhs.operator->());
            if (!leci)
                throw std::runtime_error("the expression must be a complex item");
            auto reci = dynamic_cast<riddle::complex_item *>(rhs.operator->());
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

    bool solver::matches(const riddle::expr &lhs, const riddle::expr &rhs) const noexcept
    {
        if (lhs == rhs) // the two expressions are the same..
            return true;
        else if (lhs->get_type() == get_bool_type() && rhs->get_type() == get_bool_type())
        { // the two expressions are boolean..
            auto lbi = sat->value(static_cast<bool_item &>(*lhs).get_lit());
            auto rbi = sat->value(static_cast<bool_item &>(*rhs).get_lit());
            return lbi == rbi || lbi == utils::Undefined || rbi == utils::Undefined;
        }
        else if ((lhs->get_type() == get_int_type() || lhs->get_type() == get_real_type() || lhs->get_type() == get_time_type()) && (rhs->get_type() == get_int_type() || rhs->get_type() == get_real_type() || rhs->get_type() == get_time_type()))
        { // the two expressions are arithmetics..
            auto lbi = static_cast<arith_item &>(*lhs).get_lin();
            auto rbi = static_cast<arith_item &>(*rhs).get_lin();
            if (lhs->get_type() == get_time_type() && rhs->get_type() == get_time_type()) // the two expressions are time arithmetics..
                return rdl_th.matches(lbi, rbi);
            else // the two expressions are integer or real arithmetics..
                return lra_th.matches(lbi, rbi);
        }
        else if (lhs->get_type() == get_string_type() && rhs->get_type() == get_string_type()) // the two expressions are strings..
            return static_cast<string_item &>(*lhs).get_string() == static_cast<string_item &>(*rhs).get_string();
        else if (auto lee = dynamic_cast<enum_item *>(lhs.operator->()))
        {                                                               // we are comparing an enum with something else..
            auto lee_vals = ov_th.value(lee->get_var());                // get the values of the enum..
            if (auto ree = dynamic_cast<enum_item *>(rhs.operator->())) // we are comparing enums..
            {
                auto ree_vals = ov_th.value(ree->get_var()); // get the values of the enum..
                for (const auto &lv : lee_vals)
                    for (const auto &rv : ree_vals)
                        if (lv == rv)
                            return true;
                return false;
            }
            else // we are comparing an enum with a singleton..
                for (const auto &lv : lee_vals)
                    if (lv == dynamic_cast<const utils::enum_val *>(rhs.operator->()))
                        return true;
            return false;
        }
        else if (lhs->get_type() == rhs->get_type())
        { // we are comparing two complex items..
            if (auto p = dynamic_cast<riddle::predicate *>(&lhs->get_type()))
            { // we are comparing two atoms..
                auto &leci = dynamic_cast<riddle::env &>(*lhs);
                auto &reci = dynamic_cast<riddle::env &>(*rhs);
                std::queue<riddle::predicate *> q;
                q.push(p);
                while (!q.empty())
                {
                    for (const auto &[f_name, f] : q.front()->get_fields())
                        if (!f->is_synthetic())
                            if (!matches(leci.get(f_name), reci.get(f_name)))
                                return false;
                    for (const auto &stp : q.front()->get_parents())
                        q.push(&stp.get());
                    q.pop();
                }
                return true;
            }
            else // the two items do not match..
                return false;
        }
        else // the two expressions are different..
            return false;
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
    { // we create a disjunction flaw..
        new_flaw(new disjunction_flaw(*this, get_cause(), std::move(xprs)));
    }

    riddle::expr solver::new_fact(riddle::predicate &pred)
    {
        riddle::expr fact = new atom(pred, true, semitone::lit(sat->new_var()));
        new_flaw(new atom_flaw(*this, get_cause(), fact));
        return fact;
    }

    riddle::expr solver::new_goal(riddle::predicate &pred)
    {
        riddle::expr goal = new atom(pred, false, semitone::lit(sat->new_var()));
        new_flaw(new atom_flaw(*this, get_cause(), goal));
        return goal;
    }

    bool solver::is_constant(const riddle::expr &xpr) const
    {
        if (xpr->get_type() == get_bool_type())
            return bool_value(xpr) != utils::Undefined;
        else if (xpr->get_type() == get_int_type() || xpr->get_type() == get_real_type())
        {
            auto [lb, ub] = arith_bounds(xpr);
            return lb == ub;
        }
        else if (xpr->get_type() == get_time_type())
        {
            auto [lb, ub] = time_bounds(xpr);
            return lb == ub;
        }
        else
            throw std::runtime_error("not implemented yet");

        if (is_enum(xpr))
        {
        }

        return false;
    }

    utils::lbool solver::bool_value(const riddle::expr &xpr) const { return sat->value(static_cast<bool_item &>(*xpr).get_lit()); }
    utils::inf_rational solver::arith_value(const riddle::expr &xpr) const { return lra_th.value(static_cast<arith_item &>(*xpr).get_lin()); }
    std::pair<utils::inf_rational, utils::inf_rational> solver::arith_bounds(const riddle::expr &xpr) const { return lra_th.bounds(static_cast<arith_item &>(*xpr).get_lin()); }
    utils::inf_rational solver::time_value(const riddle::expr &xpr) const { return rdl_th.bounds(static_cast<arith_item &>(*xpr).get_lin()).first; }
    std::pair<utils::inf_rational, utils::inf_rational> solver::time_bounds(const riddle::expr &xpr) const { return rdl_th.bounds(static_cast<arith_item &>(*xpr).get_lin()); }

    bool solver::is_enum(const riddle::expr &xpr) const { return dynamic_cast<enum_item *>(xpr.operator->()); }
    std::vector<riddle::expr> solver::domain(const riddle::expr &xpr) const
    {
        auto vals = ov_th.value(static_cast<enum_item &>(*xpr).get_var());
        std::vector<riddle::expr> dom; // the domain of the variable..
        dom.reserve(vals.size());
        for (auto &d : vals)
            dom.emplace_back(dynamic_cast<riddle::item *>(d));
        return dom;
    }
    void solver::prune(const riddle::expr &xpr, const riddle::expr &val)
    {
        auto alw_var = ov_th.allows(static_cast<ratio::enum_item &>(*xpr).get_var(), dynamic_cast<utils::enum_val &>(*val));
        if (!sat->new_clause({!ni, alw_var}))
            throw riddle::unsolvable_exception(); // the problem is unsolvable..
    }

    bool solver::solve()
    {
        FIRE_STARTED_SOLVING();

        try
        {
            if (sat->root_level())
            { // we make sure that gamma is at true..
                gr->build();
                gr->check();
            }
            assert(sat->value(gr->gamma) == utils::True);

            // we search for a consistent solution without flaws..
#ifdef CHECK_INCONSISTENCIES
            // we solve all the current inconsistencies..
            solve_inconsistencies();

            while (!active_flaws.empty())
            {
                assert(std::all_of(active_flaws.cbegin(), active_flaws.cend(), [this](const auto &f)
                                   { return sat->value(f->phi) == utils::True; })); // all the current flaws must be active..
                assert(std::all_of(active_flaws.cbegin(), active_flaws.cend(), [this](const auto &f)
                                   { return std::none_of(f->resolvers.cbegin(), f->resolvers.cend(), [this](const auto &r)
                                                         { return sat->value(r.get().rho) == utils::True; }); })); // none of the current flaws must have already been solved..

                // this is the next flaw (i.e. the most expensive one) to be solved..
                auto &best_flaw = **std::min_element(active_flaws.cbegin(), active_flaws.cend(), [](const auto &f0, const auto &f1)
                                                     { return f0->get_estimated_cost() > f1->get_estimated_cost(); });
                FIRE_CURRENT_FLAW(best_flaw);

                if (is_infinite(best_flaw.get_estimated_cost()))
                { // we don't know how to solve this flaw :(
                    do
                    { // we have to search..
                        next();
                    } while (std::any_of(active_flaws.cbegin(), active_flaws.cend(), [](const auto &f)
                                         { return is_infinite(f->get_estimated_cost()); }));
                    // we solve all the current inconsistencies..
                    solve_inconsistencies();
                    continue;
                }

                // this is the next resolver (i.e. the cheapest one) to be applied..
                auto &best_res = best_flaw.get_best_resolver();
                FIRE_CURRENT_RESOLVER(best_res);

                assert(!is_infinite(best_res.get_estimated_cost()));

                // we apply the resolver..
                take_decision(best_res.get_rho());

                // we solve all the current inconsistencies..
                solve_inconsistencies();
            }
#else
            do
            {
                while (!active_flaws.empty())
                {
                    assert(std::all_of(active_flaws.cbegin(), active_flaws.cend(), [this](const auto f)
                                       { return sat->value(f->phi) == utils::True; })); // all the current flaws must be active..
                    assert(std::all_of(active_flaws.cbegin(), active_flaws.cend(), [this](const auto f)
                                       { return std::none_of(f->resolvers.cbegin(), f->resolvers.cend(), [this](const auto &r)
                                                             { return sat->value(r.get().rho) == utils::True; }); })); // none of the current flaws must have already been solved..

                    // this is the next flaw (i.e. the most expensive one) to be solved..
                    auto &best_flaw = **std::min_element(active_flaws.cbegin(), active_flaws.cend(), [](const auto f0, const auto f1)
                                                         { return f0->get_estimated_cost() > f1->get_estimated_cost(); });
                    FIRE_CURRENT_FLAW(best_flaw);

                    if (is_infinite(best_flaw.get_estimated_cost()))
                    { // we don't know how to solve this flaw :(
                        do
                        { // we have to search..
                            next();
                        } while (std::any_of(active_flaws.cbegin(), active_flaws.cend(), [](const auto f)
                                             { return is_infinite(f->get_estimated_cost()); }));
                        continue;
                    }

                    // this is the next resolver (i.e. the cheapest one) to be applied..
                    auto &best_res = best_flaw.get_best_resolver();
                    FIRE_CURRENT_RESOLVER(best_res);

                    assert(!is_infinite(best_res.get_estimated_cost()));

                    // we apply the resolver..
                    take_decision(best_res.get_rho());
                }

                // we solve all the current inconsistencies..
                solve_inconsistencies();
            } while (!active_flaws.empty());
#endif
            // Hurray!! we have found a solution..
            LOG(std::to_string(trail.size()) << " (" << std::to_string(active_flaws.size()) << ")");
            FIRE_STATE_CHANGED();
            FIRE_SOLUTION_FOUND();
            return true;
        }
        catch (const riddle::unsolvable_exception &)
        { // the problem is unsolvable..
            FIRE_INCONSISTENT_PROBLEM();
            return false;
        }
    }

    void solver::take_decision(const semitone::lit &ch)
    {
        assert(sat->value(ch) == utils::Undefined);

        // we take the decision..
        if (!sat->assume(ch))
            throw riddle::unsolvable_exception();

        if (sat->root_level()) // we make sure that gamma is at true..
            gr->check();
        assert(sat->value(gr->gamma) == utils::True);

        assert(std::all_of(phis.cbegin(), phis.cend(), [this](const auto &v_fs)
                           { return std::all_of(v_fs.second.cbegin(), v_fs.second.cend(), [this](const auto &f)
                                                { return (sat->value(f->phi) != utils::False && f->get_estimated_cost() == (f->get_resolvers().empty() ? utils::rational::POSITIVE_INFINITY : f->get_best_resolver().get_estimated_cost())) || is_positive_infinite(f->get_estimated_cost()); }); }));
        assert(std::all_of(rhos.cbegin(), rhos.cend(), [this](const auto &v_rs)
                           { return std::all_of(v_rs.second.cbegin(), v_rs.second.cend(), [this](const auto &r)
                                                { return is_positive_infinite(r->get_estimated_cost()) || sat->value(r->rho) != utils::False; }); }));

        FIRE_STATE_CHANGED();
    }

    void solver::next()
    {
        assert(!sat->root_level());

        LOG("next..");
        if (!sat->next())
            throw riddle::unsolvable_exception();

        if (sat->root_level()) // we make sure that gamma is at true..
            gr->check();
        assert(sat->value(gr->gamma) == utils::True);

        assert(std::all_of(phis.cbegin(), phis.cend(), [this](const auto &v_fs)
                           { return std::all_of(v_fs.second.cbegin(), v_fs.second.cend(), [this](const auto &f)
                                                { return (sat->value(f->phi) != utils::False && f->get_estimated_cost() == (f->get_resolvers().empty() ? utils::rational::POSITIVE_INFINITY : f->get_best_resolver().get_estimated_cost())) || is_positive_infinite(f->get_estimated_cost()); }); }));
        assert(std::all_of(rhos.cbegin(), rhos.cend(), [this](const auto &v_rs)
                           { return std::all_of(v_rs.second.cbegin(), v_rs.second.cend(), [this](const auto &r)
                                                { return is_positive_infinite(r->get_estimated_cost()) || sat->value(r->rho) != utils::False; }); }));

        FIRE_STATE_CHANGED();
    }

    void solver::new_flaw(flaw_ptr f, const bool &enqueue)
    {
        if (std::any_of(f->get_causes().cbegin(), f->get_causes().cend(), [this](const auto &r)
                        { return sat->value(r.get().get_rho()) == utils::False; })) // there is no reason for introducing this flaw..
            return;

        if (!sat->root_level())
        { // we postpone the flaw's initialization..
            pending_flaws.push_back(std::move(f));
            return;
        }

        // we initialize the flaw..
        f->init(); // flaws' initialization requires being at root-level..
        FIRE_NEW_FLAW(*f);

        if (enqueue) // we enqueue the flaw..
            gr->enqueue(*f);
        else // we directly expand the flaw..
            gr->expand_flaw(*f);

        switch (sat->value(f->phi))
        {
        case utils::True: // we have a top-level (a landmark) flaw..
            if (enqueue || std::none_of(f->get_resolvers().cbegin(), f->get_resolvers().cend(), [this](const auto &r)
                                        { return sat->value(r.get().rho) == utils::True; }))
                active_flaws.insert(f.operator->()); // the flaw has not yet already been solved (e.g. it has a single resolver)..
            break;
        case utils::Undefined:      // we do not have a top-level (a landmark) flaw, nor an infeasible one..
            bind(variable(f->phi)); // we listen for the flaw to become active..
            break;
        }

        phis[variable(f->phi)].emplace_back(std::move(f));
    }

    void solver::new_resolver(resolver_ptr r)
    {
        FIRE_NEW_RESOLVER(*r);
        if (sat->value(r->rho) == utils::Undefined) // we do not have a top-level (a landmark) resolver, nor an infeasible one..
            bind(variable(r->rho));                 // we listen for the resolver to become inactive..

        rhos[variable(r->rho)].push_back(std::move(r));
    }

    void solver::new_causal_link(flaw &f, resolver &r)
    {
        FIRE_CAUSAL_LINK_ADDED(f, r);
        r.preconditions.push_back(f);
        f.supports.push_back(r);
        // activating the resolver requires the activation of the flaw..
        [[maybe_unused]] bool new_clause = sat->new_clause({!r.rho, f.phi});
        assert(new_clause);
        // we introduce an ordering constraint..
        [[maybe_unused]] bool new_dist = sat->new_clause({!r.rho, idl_th.new_distance(r.get_flaw().position, f.position, 0)});
        assert(new_dist);
    }

    void solver::expand_flaw(flaw &f)
    {
        assert(!f.expanded);

        // we expand the flaw..
        f.expand();

        // we apply the flaw's resolvers..
        for (const auto &r : f.resolvers)
            apply_resolver(r);

        if (!sat->propagate())
            throw riddle::unsolvable_exception(); // the problem is unsolvable..

        // we clean up already solved flaws..
        if (sat->value(f.phi) == utils::True && std::any_of(f.resolvers.cbegin(), f.resolvers.cend(), [this](const auto &r)
                                                            { return sat->value(r.get().rho) == utils::True; }))
            active_flaws.erase(&f); // this flaw has already been solved..
    }

    void solver::apply_resolver(resolver &r)
    {
        res = &r;      // we write down the resolver so that new flaws know their cause..
        set_ni(r.rho); // we temporally set the ni variable..

        try
        { // we apply the resolver..
            r.apply();
        }
        catch (const riddle::inconsistency_exception &)
        { // the resolver is inapplicable..
            if (!sat->new_clause({!r.rho}))
                throw riddle::unsolvable_exception();
        }

        // we make some cleanings..
        restore_ni();
        res = nullptr;
    }

    void solver::set_cost(flaw &f, utils::rational cost)
    {
        assert(f.est_cost != cost);
        if (!trail.empty()) // we store the current flaw's estimated cost, if not already stored, for allowing backtracking..
            trail.back().old_f_costs.try_emplace(&f, f.est_cost);

        // we update the flaw's estimated cost..
        f.est_cost = cost;
        FIRE_FLAW_COST_CHANGED(f);
    }

    void solver::solve_inconsistencies()
    {
        // all the current inconsistencies..
        auto incs = get_incs();

        while (!incs.empty())
            if (const auto &uns_flw = std::find_if(incs.cbegin(), incs.cend(), [](const auto &v)
                                                   { return v.empty(); });
                uns_flw != incs.cend())
            { // we have an unsolvable flaw..
                // we backtrack..
                next();
                // we re-collect all the inconsistencies from all the smart-types..
                incs = get_incs();
            }
            else if (const auto &det_flw = std::find_if(incs.cbegin(), incs.cend(), [](const auto &v)
                                                        { return v.size() == 1; });
                     det_flw != incs.cend())
            { // we have deterministic flaw: i.e., a flaw with a single resolver..
                assert(sat->value(det_flw->front().first) != utils::False);
                if (sat->value(det_flw->front().first) == utils::Undefined)
                { // we can learn something from it..
                    std::vector<semitone::lit> learnt;
                    learnt.reserve(trail.size() + 1);
                    learnt.push_back(det_flw->front().first);
                    for (const auto &l : sat->get_decisions())
                        learnt.push_back(!l);
                    record(learnt);
                    if (!sat->propagate())
                        throw riddle::unsolvable_exception();

                    if (sat->root_level()) // we make sure that gamma is at true..
                        gr->check();
                    assert(sat->value(gr->gamma) == utils::True);
                }

                // we re-collect all the inconsistencies from all the smart-types..
                incs = get_incs();
            }
            else
            { // we have to take a decision..
                std::vector<std::pair<semitone::lit, double>> bst_inc;
                double k_inv = std::numeric_limits<double>::infinity();
                for (const auto &inc : incs)
                {
                    double bst_commit = std::numeric_limits<double>::infinity();
                    for ([[maybe_unused]] const auto &[choice, commit] : inc)
                        if (commit < bst_commit)
                            bst_commit = commit;
                    double c_k_inv = 0;
                    for ([[maybe_unused]] const auto &[choice, commit] : inc)
                        c_k_inv += 1l / (1l + (commit - bst_commit));
                    if (c_k_inv < k_inv)
                    {
                        k_inv = c_k_inv;
                        bst_inc = inc;
                    }
                }

                // we select the best choice (i.e. the least committing one) from those available for the best flaw..
                take_decision(std::min_element(bst_inc.cbegin(), bst_inc.cend(), [](const auto &ch0, const auto &ch1)
                                               { return ch0.second < ch1.second; })
                                  ->first);

                // we re-collect all the inconsistencies from all the smart-types..
                incs = get_incs();
            }
    }

    std::vector<std::vector<std::pair<semitone::lit, double>>> solver::get_incs()
    {
        std::vector<std::vector<std::pair<semitone::lit, double>>> incs;
        // we collect all the inconsistencies from all the smart-types..
        for (const auto &smrtp : smart_types)
        {
            const auto c_incs = smrtp->get_current_incs();
            incs.insert(incs.cend(), c_incs.cbegin(), c_incs.cend());
        }
        assert(std::all_of(incs.cbegin(), incs.cend(), [](const auto &inc)
                           { return std::all_of(inc.cbegin(), inc.cend(), [](const auto &ch)
                                                { return std::isfinite(ch.second); }); }));
        return incs;
    }

    void solver::reset_smart_types()
    {
        // we reset the smart types..
        smart_types.clear();
        // we seek for the existing smart types..
        std::queue<riddle::complex_type *> q;
        for (const auto &t : get_types())
            if (auto ct = dynamic_cast<riddle::complex_type *>(&t.get()))
                q.push(ct);
        while (!q.empty())
        {
            auto ct = q.front();
            q.pop();
            if (const auto st = dynamic_cast<smart_type *>(ct); st)
                smart_types.emplace_back(st);
            for (const auto &t : ct->get_types())
                if (auto c_ct = dynamic_cast<riddle::complex_type *>(&t.get()))
                    q.push(c_ct);
        }
    }

    bool solver::propagate(const semitone::lit &p)
    {
        assert(cnfl.empty());
        assert(phis.count(variable(p)) || rhos.count(variable(p)));

        if (const auto at_phis_p = phis.find(variable(p)); at_phis_p != phis.cend())
            switch (sat->value(at_phis_p->first))
            {
            case utils::True: // some flaws have been activated..
                for (const auto &f : at_phis_p->second)
                {
                    assert(!active_flaws.count(f.operator->()));
                    if (!sat->root_level())
                        trail.back().new_flaws.insert(f.operator->());
                    if (std::none_of(f->resolvers.cbegin(), f->resolvers.cend(), [this](const auto &r)
                                     { return sat->value(r.get().rho) == utils::True; }))
                        active_flaws.insert(f.operator->()); // this flaw has been activated and not yet accidentally solved..
                    else if (!sat->root_level())
                        trail.back().solved_flaws.insert(f.operator->()); // this flaw has been accidentally solved..
                    gr->activated_flaw(*f);
                }
                break;
            case utils::False: // some flaws have been negated..
                for (const auto &f : at_phis_p->second)
                {
                    assert(!active_flaws.count(f.operator->()));
                    gr->negated_flaw(*f);
                }
                break;
            }

        if (const auto at_rhos_p = rhos.find(variable(p)); at_rhos_p != rhos.cend())
            switch (sat->value(at_rhos_p->first))
            {
            case utils::True: // some resolvers have been activated..
                for (const auto &r : at_rhos_p->second)
                {
                    if (active_flaws.erase(&r->f) && !sat->root_level()) // this resolver has been activated, hence its effect flaw has been resolved (notice that we remove its effect only in case it was already active)..
                        trail.back().solved_flaws.insert(&r->f);
                    gr->activated_resolver(*r);
                }
                break;
            case utils::False: // some resolvers have been negated..
                for (const auto &r : at_rhos_p->second)
                    gr->negated_resolver(*r);
                break;
            }

        return true;
    }

    bool solver::check()
    {
        assert(cnfl.empty());
        assert(std::all_of(active_flaws.cbegin(), active_flaws.cend(), [this](const auto f)
                           { return sat->value(f->phi) == utils::True; }));
        assert(std::all_of(phis.cbegin(), phis.cend(), [this](const auto &v_fs)
                           { return std::all_of(v_fs.second.cbegin(), v_fs.second.cend(), [this](const auto &f)
                                                { return sat->value(f->phi) != utils::False || is_positive_infinite(f->get_estimated_cost()); }); }));
        assert(std::all_of(rhos.cbegin(), rhos.cend(), [this](const auto &v_rs)
                           { return std::all_of(v_rs.second.cbegin(), v_rs.second.cend(), [this](const auto &r)
                                                { return sat->value(r->rho) != utils::False || is_positive_infinite(r->get_estimated_cost()); }); }));
        return true;
    }

    void solver::push()
    {
        LOG(std::to_string(trail.size()) << " (" << std::to_string(active_flaws.size()) << ")");

        trail.emplace_back(); // we add a new layer to the trail..
        gr->push();           // we push the graph..
    }

    void solver::pop()
    {
        LOG(std::to_string(trail.size()) << " (" << std::to_string(active_flaws.size()) << ")");

        // we reintroduce the solved flaw..
        for (const auto &f : trail.back().solved_flaws)
            active_flaws.insert(f);

        // we erase the new flaws..
        for (const auto &f : trail.back().new_flaws)
            active_flaws.erase(f);

        // we restore the flaws' estimated costs..
        for (const auto &[f, cost] : trail.back().old_f_costs)
        {
            // assert(f.first->est_cost != cost);
            f->est_cost = cost;
            FIRE_FLAW_COST_CHANGED(*f);
        }

        trail.pop_back(); // we remove the last layer from the trail..
        gr->pop();        // we pop the graph..

        LOG(std::to_string(trail.size()) << " (" << std::to_string(active_flaws.size()) << ")");
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

    json::json to_json(const solver &rhs) noexcept
    {
        // we collect all the items and atoms..
        std::set<riddle::item *> all_items;
        std::set<atom *> all_atoms;
        for (const auto &pred : rhs.get_predicates())
            for (const auto &a : pred.get().get_instances())
                all_atoms.insert(static_cast<atom *>(&*a));
        std::queue<riddle::complex_type *> q;
        for (const auto &tp : rhs.get_types())
            if (!tp.get().is_primitive())
            {
                if (auto ct = dynamic_cast<riddle::complex_type *>(&tp.get()))
                    q.push(ct);
                else if (auto et = dynamic_cast<riddle::enum_type *>(&tp.get()))
                    for (const auto &etv : et->get_all_values())
                        all_items.insert(&*etv);
                else
                    assert(false);
            }
        while (!q.empty())
        {
            for (const auto &i : q.front()->get_instances())
                all_items.insert(&*i);
            for (const auto &pred : q.front()->get_predicates())
                for (const auto &a : pred.get().get_instances())
                    all_atoms.insert(static_cast<atom *>(&*a));
            for (const auto &tp : q.front()->get_types())
                if (auto ct = dynamic_cast<riddle::complex_type *>(&tp.get()))
                    q.push(ct);
                else if (auto et = dynamic_cast<riddle::enum_type *>(&tp.get()))
                    for (const auto &etv : et->get_all_values())
                        all_items.insert(&*etv);
                else
                    assert(false);
            q.pop();
        }

        json::json j_core;

        if (!all_items.empty())
        {
            json::json j_itms(json::json_type::array);
            for (const auto &itm : all_items)
                j_itms.push_back(to_json(*itm));
            j_core["items"] = std::move(j_itms);
        }

        if (!all_atoms.empty())
        {
            json::json j_atms(json::json_type::array);
            for (const auto &atm : all_atoms)
                j_atms.push_back(to_json(*atm));
            j_core["atoms"] = std::move(j_atms);
        }

        if (!rhs.get_vars().empty())
            j_core["exprs"] = to_json(rhs.get_vars());
        return j_core;
    }

    json::json to_timelines(solver &rhs) noexcept
    {
        json::json tls(json::json_type::array);

        // for each pulse, the atoms starting at that pulse..
        std::map<utils::inf_rational, std::set<atom *>> starting_atoms;
        // all the pulses of the timeline..
        std::set<utils::inf_rational> pulses;
        for (const auto &pred : rhs.get_predicates())
            if (rhs.is_impulse(pred.get()) || rhs.is_interval(pred.get()))
                for (const auto &atm : pred.get().get_instances())
                    if (&atm->get_type().get_core() != &rhs && rhs.get_sat_core().value(static_cast<atom &>(*atm).get_sigma()) == utils::True)
                    {
                        utils::inf_rational start = rhs.get_core().arith_value(rhs.is_impulse(pred.get()) ? static_cast<atom &>(*atm).get(RATIO_AT) : static_cast<atom &>(*atm).get(RATIO_START));
                        starting_atoms[start].insert(dynamic_cast<atom *>(&*atm));
                        pulses.insert(start);
                    }
        if (!starting_atoms.empty())
        {
            json::json slv_tl;
            slv_tl["id"] = get_id(rhs);
            slv_tl["name"] = "solver";
            json::json j_atms(json::json_type::array);
            for (const auto &p : pulses)
                for (const auto &atm : starting_atoms.at(p))
                    j_atms.push_back(to_json(*atm));
            slv_tl["values"] = std::move(j_atms);
            tls.push_back(std::move(slv_tl));
        }

        std::queue<riddle::complex_type *> q;
        for (const auto &tp : rhs.get_types())
            if (!tp.get().is_primitive())
                q.push(static_cast<riddle::complex_type *>(&tp.get()));

        while (!q.empty())
        {
            if (auto tl_tp = dynamic_cast<timeline *>(q.front()))
            {
                auto j_tls = tl_tp->extract();
                for (size_t i = 0; i < j_tls.size(); ++i)
                    tls.push_back(std::move(j_tls[i]));
            }
            for (const auto &tp : q.front()->get_types())
                q.push(static_cast<riddle::complex_type *>(&tp.get()));
            q.pop();
        }

        return tls;
    }

    json::json to_json(const riddle::item &rhs) noexcept
    {
        json::json j_itm;
        j_itm["id"] = get_id(rhs);
        j_itm["type"] = rhs.get_type().get_full_name();
#ifdef COMPUTE_NAMES
        auto &slv = static_cast<const solver &>(rhs.get_type().get_core());
        auto name = slv.guess_name(rhs);
        if (!name.empty())
            j_itm["name"] = name;
#endif
        if (auto ci = dynamic_cast<const riddle::complex_item *>(&rhs))
        {
            if (dynamic_cast<const riddle::enum_item *>(&rhs))
                j_itm["val"] = value(rhs);
            if (!ci->get_vars().empty())
                j_itm["exprs"] = to_json(ci->get_vars());
            if (auto atm = dynamic_cast<const atom *>(&rhs))
                j_itm["sigma"] = variable(atm->get_sigma());
        }
        else
            j_itm["value"] = value(rhs);

        return j_itm;
    }
    json::json to_json(const std::map<std::string, riddle::expr> &vars) noexcept
    {
        json::json j_exprs;
        for (const auto &[xpr_name, xpr] : vars)
        {
            json::json j_var;
            j_var["name"] = xpr_name;
            j_var["type"] = xpr->get_type().get_full_name();
            j_var["value"] = value(*xpr);
            j_exprs.push_back(std::move(j_var));
        }
        return j_exprs;
    }

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
                vals.push_back(get_id(dynamic_cast<riddle::item &>(*v)));
            j_val["vals"] = std::move(vals);
            return j_val;
        }
        else
            return get_id(itm);
    }

    std::string to_string(const atom &atm) noexcept
    {
        std::string str = (atm.is_fact() ? "fact" : "goal");
        str += " σ" + std::to_string(variable(atm.get_sigma()));
        str += " " + atm.get_type().get_name();
        switch (static_cast<const solver &>(atm.get_core()).get_sat_core().value(atm.get_sigma()))
        {
        case utils::True: // the atom is active..
            str += " (active)";
            break;
        case utils::False: // the atom is unified..
            str += " (unified)";
            break;
        case utils::Undefined: // the atom is inactive..
            str += " (inactive)";
            break;
        }
        return str;
    }
} // namespace ratio
