#include "solver.h"
#include "init.h"
#include "item.h"
#include "predicate.h"
#include "field.h"
#include "conjunction.h"
#include "causal_graph.h"
#include "bool_flaw.h"
#include "disj_flaw.h"
#include "enum_flaw.h"
#include "disjunction_flaw.h"
#include "atom_flaw.h"
#include "smart_type.h"
#include "agent.h"
#include "state_variable.h"
#include "reusable_resource.h"
#include "consumable_resource.h"
#if defined(H_MAX) || defined(H_ADD)
#include "h_1.h"
#define HEURISTIC std::make_unique<h_1>()
#endif
#ifdef BUILD_LISTENERS
#include "solver_listener.h"
#endif
#include <algorithm>
#include <cassert>

namespace ratio::solver
{
    ORATIOSOLVER_EXPORT solver::solver(const bool &i) : solver(HEURISTIC, i) {}
    ORATIOSOLVER_EXPORT solver::solver(std::unique_ptr<causal_graph> c_gr, const bool &i) : theory(std::make_shared<semitone::sat_core>()), lra_th(sat), ov_th(sat), idl_th(sat), rdl_th(sat), gr(std::move(c_gr))
    {
        gr->init(*this); // we initialize the causal graph..
        if (i)           // we initializa the solver..
            init();
    }
    ORATIOSOLVER_EXPORT solver::~solver() {}

    ORATIOSOLVER_EXPORT void solver::read(const std::string &script)
    {
        // we read the script..
        core::read(script);
        // we reset the smart-types if some new smart-type has been added with the previous script..
        reset_smart_types();

        if (!sat->propagate())
            throw ratio::core::unsolvable_exception();
        FIRE_STATE_CHANGED();
    }
    ORATIOSOLVER_EXPORT void solver::read(const std::vector<std::string> &files)
    {
        // we read the files..
        core::read(files);
        // we reset the smart-types if some new smart-type has been added with the previous files..
        reset_smart_types();

        if (!sat->propagate())
            throw ratio::core::unsolvable_exception();
        FIRE_STATE_CHANGED();
    }

    ORATIOSOLVER_EXPORT void solver::init() noexcept
    {
        read(INIT_STRING);
        imp_pred = &get_predicate(RATIO_IMPULSE);
        int_pred = &get_predicate(RATIO_INTERVAL);
        new_type(std::make_unique<agent>(*this));
        new_type(std::make_unique<state_variable>(*this));
        new_type(std::make_unique<reusable_resource>(*this));
        new_type(std::make_unique<consumable_resource>(*this));
        FIRE_STATE_CHANGED();
    }

    ORATIOSOLVER_EXPORT ratio::core::expr solver::new_bool() noexcept
    { // we create a new boolean expression..
        auto b_xpr = std::make_shared<ratio::core::bool_item>(get_bool_type(), semitone::lit(sat->new_var()));
        // we create a new boolean flaw..
        new_flaw(std::make_unique<bool_flaw>(*this, get_cause(), *b_xpr));
        return b_xpr;
    }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::new_int() noexcept { return std::make_shared<ratio::core::arith_item>(get_int_type(), semitone::lin(lra_th.new_var(), semitone::rational::ONE)); }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::new_real() noexcept { return std::make_shared<ratio::core::arith_item>(get_real_type(), semitone::lin(lra_th.new_var(), semitone::rational::ONE)); }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::new_time_point() noexcept { return std::make_shared<ratio::core::arith_item>(get_time_type(), semitone::lin(lra_th.new_var(), semitone::rational::ONE)); }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::new_string() noexcept { return std::make_shared<ratio::core::string_item>(get_string_type(), ""); }

    ORATIOSOLVER_EXPORT ratio::core::expr solver::new_enum(ratio::core::type &tp, const std::vector<ratio::core::expr> &allowed_vals)
    {
        assert(&tp != &get_bool_type());
        assert(&tp != &get_int_type());
        assert(&tp != &get_real_type());
        assert(&tp != &get_time_type());
        if (allowed_vals.empty())
            throw ratio::core::inconsistency_exception();

        std::vector<semitone::var_value *> vals;
        vals.reserve(allowed_vals.size());
        for (const auto &i : allowed_vals)
            vals.push_back(i.get());

        // we create a new enum expression..
        // notice that we do not enforce the exct_one constraint!
        auto e_xpr = std::make_shared<ratio::core::enum_item>(tp, ov_th.new_var(vals, false));
        if (allowed_vals.size() > 1) // we create a new enum flaw..
            new_flaw(std::make_unique<enum_flaw>(*this, get_cause(), *e_xpr));
        return e_xpr;
    }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::get_enum(ratio::core::enum_item &var, const std::string &name)
    {
        auto vs = ov_th.value(var.get_var());
        assert(vs.size() > 1);
        std::unordered_map<ratio::core::item *, std::vector<semitone::lit>> val_vars;
        for (const auto &val : vs)
            val_vars[&*static_cast<ratio::core::complex_item *>(val)->get(name)].push_back(ov_th.allows(var.get_var(), *val));
        std::vector<semitone::lit> c_vars;
        std::vector<semitone::var_value *> c_vals;
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

        if (&var.get_type() == &get_bool_type())
        {
            auto b = new_bool();
            [[maybe_unused]] bool nc;
            for (size_t i = 0; i < c_vars.size(); ++i)
            {
                nc = sat->new_clause({!c_vars[i], sat->new_eq(static_cast<ratio::core::bool_item &>(*c_vals[i]).get_value(), static_cast<ratio::core::bool_item &>(*b).get_value())});
                assert(nc);
            }
            return b;
        }
        else if (&var.get_type() == &get_int_type())
        {
            auto min = semitone::inf_rational(semitone::rational::POSITIVE_INFINITY);
            auto max = semitone::inf_rational(semitone::rational::NEGATIVE_INFINITY);
            for (const auto &val : c_vals)
            {
                const auto [lb, ub] = lra_th.bounds(static_cast<ratio::core::arith_item &>(*val).get_value());
                if (min > lb)
                    min = lb;
                if (max < ub)
                    max = ub;
            }
            assert(min.get_infinitesimal() == semitone::rational::ZERO);
            assert(max.get_infinitesimal() == semitone::rational::ZERO);
            assert(min.get_rational().denominator() == 1);
            assert(max.get_rational().denominator() == 1);
            if (min == max) // we have a constant..
                return ratio::core::core::new_int(min.get_rational().numerator());
            else
            { // we need to create a new (bounded) integer variable..
                auto ie = new_int();
                [[maybe_unused]] bool nc;
                for (size_t i = 0; i < c_vars.size(); ++i)
                {
                    nc = sat->new_clause({!c_vars[i], lra_th.new_eq(static_cast<ratio::core::arith_item &>(*ie).get_value(), static_cast<ratio::core::arith_item &>(*c_vals[i]).get_value())});
                    assert(nc);
                }
                // we impose some bounds which might help propagation..
                nc = sat->new_clause({lra_th.new_geq(static_cast<ratio::core::arith_item &>(*ie).get_value(), semitone::lin(min.get_rational()))});
                assert(nc);
                nc = sat->new_clause({lra_th.new_leq(static_cast<ratio::core::arith_item &>(*ie).get_value(), semitone::lin(max.get_rational()))});
                assert(nc);
                return ie;
            }
        }
        else if (&var.get_type() == &get_real_type())
        {
            auto min = semitone::inf_rational(semitone::rational::POSITIVE_INFINITY);
            auto max = semitone::inf_rational(semitone::rational::NEGATIVE_INFINITY);
            for (const auto &val : c_vals)
            {
                const auto [lb, ub] = lra_th.bounds(static_cast<ratio::core::arith_item &>(*val).get_value());
                if (min > lb)
                    min = lb;
                if (max < ub)
                    max = ub;
            }
            assert(min.get_infinitesimal() == semitone::rational::ZERO);
            assert(max.get_infinitesimal() == semitone::rational::ZERO);
            if (min == max) // we have a constant..
                return ratio::core::core::new_real(min.get_rational());
            else
            { // we need to create a new (bounded) real variable..
                auto re = new_real();
                [[maybe_unused]] bool nc;
                for (size_t i = 0; i < c_vars.size(); ++i)
                {
                    nc = sat->new_clause({!c_vars[i], lra_th.new_eq(static_cast<ratio::core::arith_item &>(*re).get_value(), static_cast<ratio::core::arith_item &>(*c_vals[i]).get_value())});
                    assert(nc);
                }
                // we impose some bounds which might help propagation..
                nc = sat->new_clause({lra_th.new_geq(static_cast<ratio::core::arith_item &>(*re).get_value(), semitone::lin(min.get_rational()))});
                assert(nc);
                nc = sat->new_clause({lra_th.new_leq(static_cast<ratio::core::arith_item &>(*re).get_value(), semitone::lin(max.get_rational()))});
                assert(nc);
                return re;
            }
        }
        else if (&var.get_type() == &get_time_type())
        {
            auto min = semitone::inf_rational(semitone::rational::POSITIVE_INFINITY);
            auto max = semitone::inf_rational(semitone::rational::NEGATIVE_INFINITY);
            for (const auto &val : c_vals)
            {
                const auto [lb, ub] = rdl_th.bounds(static_cast<ratio::core::arith_item &>(*val).get_value());
                if (min > lb)
                    min = lb;
                if (max < ub)
                    max = ub;
            }
            assert(min.get_infinitesimal() == semitone::rational::ZERO);
            assert(max.get_infinitesimal() == semitone::rational::ZERO);
            if (min == max) // we have a constant..
                return ratio::core::core::new_time_point(min.get_rational());
            else
            { // we need to create a new time-point variable..
                auto tm_pt = new_time_point();
                [[maybe_unused]] bool nc;
                for (size_t i = 0; i < c_vars.size(); ++i)
                {
                    nc = sat->new_clause({!c_vars[i], rdl_th.new_eq(static_cast<ratio::core::arith_item &>(*tm_pt).get_value(), static_cast<ratio::core::arith_item &>(*c_vals[i]).get_value())});
                    assert(nc);
                }
                // we impose some bounds which might help propagation..
                nc = sat->new_clause({rdl_th.new_geq(static_cast<ratio::core::arith_item &>(*tm_pt).get_value(), semitone::lin(min.get_rational()))});
                assert(nc);
                nc = sat->new_clause({rdl_th.new_leq(static_cast<ratio::core::arith_item &>(*tm_pt).get_value(), semitone::lin(max.get_rational()))});
                assert(nc);
                return tm_pt;
            }
        }
        else
            return std::make_shared<ratio::core::enum_item>(var.get_type().get_field(name).get_type(), ov_th.new_var(c_vars, c_vals));
    }
    ORATIOSOLVER_EXPORT void solver::remove(ratio::core::expr &var, semitone::var_value &val)
    {
        auto alw_var = ov_th.allows(static_cast<ratio::core::enum_item &>(*var).get_var(), val);
        if (!sat->new_clause({!ni, !alw_var}))
            throw ratio::core::unsolvable_exception();
    }

    ORATIOSOLVER_EXPORT ratio::core::expr solver::negate(const ratio::core::expr &var) noexcept { return std::make_shared<ratio::core::bool_item>(get_bool_type(), !static_cast<ratio::core::bool_item &>(*var).get_value()); }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::conj(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        std::vector<semitone::lit> lits;
        for (const auto &bex : exprs)
            lits.push_back(static_cast<ratio::core::bool_item &>(*bex).get_value());
        return std::make_shared<ratio::core::bool_item>(get_bool_type(), sat->new_conj(std::move(lits)));
    }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::disj(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        std::vector<semitone::lit> lits;
        for (const auto &bex : exprs)
            lits.push_back(static_cast<ratio::core::bool_item &>(*bex).get_value());

        auto d_xpr = std::make_shared<ratio::core::bool_item>(get_bool_type(), sat->new_disj(lits));

        if (exprs.size() > 1) // we create a new var flaw..
            new_flaw(std::make_unique<disj_flaw>(*this, get_cause(), std::move(lits)));

        return d_xpr;
    }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::exct_one(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        std::vector<semitone::lit> lits;
        for (const auto &bex : exprs)
            lits.push_back(static_cast<ratio::core::bool_item &>(*bex).get_value());
        return std::make_shared<ratio::core::bool_item>(get_bool_type(), sat->new_exct_one(std::move(lits)));
    }

    ORATIOSOLVER_EXPORT ratio::core::expr solver::add(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        assert(exprs.size() > 1);
        semitone::lin l;
        for (const auto &aex : exprs)
            l += static_cast<ratio::core::arith_item &>(*aex).get_value();
        return std::make_shared<ratio::core::arith_item>(get_type(exprs), l);
    }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::sub(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        assert(exprs.size() > 1);
        semitone::lin l;
        for (auto xpr_it = exprs.cbegin(); xpr_it != exprs.cend(); ++xpr_it)
            if (xpr_it == exprs.cbegin())
                l += static_cast<ratio::core::arith_item &>(**xpr_it).get_value();
            else
                l -= static_cast<ratio::core::arith_item &>(**xpr_it).get_value();
        return std::make_shared<ratio::core::arith_item>(get_type(exprs), l);
    }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::mult(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        assert(exprs.size() > 1);
        if (auto var_it = std::find_if(exprs.cbegin(), exprs.cend(), [this](const auto &ae)
                                       { return is_constant(static_cast<ratio::core::arith_item &>(*ae)); });
            var_it != exprs.cend())
        {
            auto c_xpr = *var_it;
            semitone::lin l = static_cast<ratio::core::arith_item &>(*c_xpr).get_value();
            for (const auto &xpr : exprs)
                if (xpr != c_xpr)
                {
                    assert(is_constant(static_cast<ratio::core::arith_item &>(*xpr)) && "non-linear expression..");
                    assert(lra_th.value(static_cast<ratio::core::arith_item &>(*xpr).get_value()).get_infinitesimal() == semitone::rational::ZERO);
                    l *= lra_th.value(static_cast<ratio::core::arith_item &>(*xpr).get_value()).get_rational();
                }
            return std::make_shared<ratio::core::arith_item>(get_type(exprs), l);
        }
        else
        {
            semitone::lin l = static_cast<ratio::core::arith_item &>(**exprs.cbegin()).get_value();
            for (auto xpr_it = ++exprs.cbegin(); xpr_it != exprs.cend(); ++xpr_it)
            {
                assert(lra_th.value(static_cast<ratio::core::arith_item &>(**xpr_it).get_value()).get_infinitesimal() == semitone::rational::ZERO);
                l *= lra_th.value(static_cast<ratio::core::arith_item &>(**xpr_it).get_value()).get_rational();
            }
            return std::make_shared<ratio::core::arith_item>(get_type(exprs), l);
        }
    }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::div(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        assert(exprs.size() > 1);
        assert(std::all_of(++exprs.cbegin(), exprs.cend(), [this](const auto &ae)
                           { return is_constant(static_cast<ratio::core::arith_item &>(*ae)); }) &&
               "non-linear expression..");
        assert(lra_th.value(static_cast<ratio::core::arith_item &>(*exprs[1]).get_value()).get_infinitesimal() == semitone::rational::ZERO);
        auto c = lra_th.value(static_cast<ratio::core::arith_item &>(*exprs[1]).get_value()).get_rational();
        for (size_t i = 2; i < exprs.size(); ++i)
        {
            assert(lra_th.value(static_cast<ratio::core::arith_item &>(*exprs[i]).get_value()).get_infinitesimal() == semitone::rational::ZERO);
            c *= lra_th.value(static_cast<ratio::core::arith_item &>(*exprs[i]).get_value()).get_rational();
        }
        return std::make_shared<ratio::core::arith_item>(get_type(exprs), static_cast<ratio::core::arith_item &>(*exprs.at(0)).get_value() / c);
    }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::minus(const ratio::core::expr &ex) noexcept { return std::make_shared<ratio::core::arith_item>(ex->get_type(), -static_cast<ratio::core::arith_item &>(*ex).get_value()); }

    ORATIOSOLVER_EXPORT ratio::core::expr solver::lt(const ratio::core::expr &left, const ratio::core::expr &right) noexcept
    {
        if (&get_type({left, right}) == &get_time_type())
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), rdl_th.new_lt(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
        else
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), lra_th.new_lt(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
    }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::leq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept
    {
        if (&get_type({left, right}) == &get_time_type())
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), rdl_th.new_leq(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
        else
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), lra_th.new_leq(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
    }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::eq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept { return std::make_shared<ratio::core::bool_item>(get_bool_type(), eq(*left, *right)); }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::geq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept
    {
        if (&get_type({left, right}) == &get_time_type())
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), rdl_th.new_geq(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
        else
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), lra_th.new_geq(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
    }
    ORATIOSOLVER_EXPORT ratio::core::expr solver::gt(const ratio::core::expr &left, const ratio::core::expr &right) noexcept
    {
        if (&get_type({left, right}) == &get_time_type())
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), rdl_th.new_gt(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
        else
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), lra_th.new_gt(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
    }

    semitone::lit solver::eq(ratio::core::item &left, ratio::core::item &right) noexcept
    {
        if (&left == &right) // the two items are the same item..
            return semitone::TRUE_lit;
        else if (&left.get_type() == &get_bool_type() && &right.get_type() == &get_bool_type()) // we are comparing boolean expressions..
            return sat->new_eq(static_cast<ratio::core::bool_item &>(left).get_value(), static_cast<ratio::core::bool_item &>(right).get_value());
        else if (&left.get_type() == &get_string_type() && &right.get_type() == &get_string_type()) // we are comparing string expressions..
            return static_cast<ratio::core::string_item &>(left).get_value() == static_cast<ratio::core::string_item &>(right).get_value() ? semitone::TRUE_lit : semitone::FALSE_lit;
        else if ((&left.get_type() == &get_int_type() || &left.get_type() == &get_real_type() || &left.get_type() == &get_time_type()) && (&right.get_type() == &get_int_type() || &right.get_type() == &get_real_type() || &right.get_type() == &get_time_type()))
        { // we are comparing arithmetic expressions..
            if (&get_type(std::vector<const ratio::core::item *>({&left, &right})) == &get_time_type())
                return rdl_th.new_eq(static_cast<ratio::core::arith_item &>(left).get_value(), static_cast<ratio::core::arith_item &>(right).get_value());
            else
                return lra_th.new_eq(static_cast<ratio::core::arith_item &>(left).get_value(), static_cast<ratio::core::arith_item &>(right).get_value());
        }
        else if (auto lee = dynamic_cast<ratio::core::enum_item *>(&left))
        {
            if (auto ree = dynamic_cast<ratio::core::enum_item *>(&right)) // we are comparing enums..
                return ov_th.new_eq(lee->get_var(), ree->get_var());
            else
                return ov_th.allows(lee->get_var(), right);
        }
        else if (dynamic_cast<ratio::core::enum_item *>(&right))
            return eq(right, left); // we swap, for simplifying code..
        else if (auto leci = dynamic_cast<ratio::core::complex_item *>(&left))
        {
            if (auto reci = dynamic_cast<ratio::core::complex_item *>(&right))
            { // we are comparing complex items..
                std::vector<semitone::lit> eqs;
                std::queue<ratio::core::type *> q;
                q.push(&left.get_type());
                while (!q.empty())
                {
                    for (const auto &[f_name, f] : q.front()->get_fields())
                        if (!f->is_synthetic())
                        {
                            auto c_eq = eq(*leci->get(f_name), *reci->get(f_name));
                            if (sat->value(c_eq) == semitone::False)
                                return semitone::FALSE_lit;
                            eqs.push_back(c_eq);
                        }
                    for (const auto &stp : q.front()->get_supertypes())
                        q.push(stp);
                    q.pop();
                }
                switch (eqs.size())
                {
                case 0:
                    return semitone::FALSE_lit;
                case 1:
                    return *eqs.cbegin();
                default:
                    return sat->new_conj(std::move(eqs));
                }
            }
            else
                return semitone::FALSE_lit;
        }
        else
            return semitone::FALSE_lit;
    }
    bool solver::matches(ratio::core::item &left, ratio::core::item &right) noexcept
    {
        if (&left == &right)
            return true; // the two items are the same item..
        else if (&left.get_type() != &right.get_type())
            return false; // the two items have different types..
        else if (&left.get_type() == &get_bool_type() && &right.get_type() == &get_bool_type())
        { // we are comparing boolean expressions..
            auto l_val = sat->value(static_cast<ratio::core::bool_item &>(left).get_value());
            auto r_val = sat->value(static_cast<ratio::core::bool_item &>(right).get_value());
            return l_val == r_val || l_val == semitone::Undefined || r_val == semitone::Undefined;
        }
        else if (&left.get_type() == &get_string_type() && &right.get_type() == &get_string_type()) // we are comparing string expressions..
            return static_cast<ratio::core::string_item &>(left).get_value() == static_cast<ratio::core::string_item &>(right).get_value();
        else if ((&left.get_type() == &get_int_type() || &left.get_type() == &get_real_type() || &left.get_type() == &get_time_type()) && (&right.get_type() == &get_int_type() || &right.get_type() == &get_real_type() || &right.get_type() == &get_time_type()))
        { // we are comparing arithmetic expressions..
            if (&get_type(std::vector<const ratio::core::item *>({&left, &right})) == &get_time_type())
                return rdl_th.matches(static_cast<ratio::core::arith_item &>(left).get_value(), static_cast<ratio::core::arith_item &>(right).get_value());
            else
                return lra_th.matches(static_cast<ratio::core::arith_item &>(left).get_value(), static_cast<ratio::core::arith_item &>(right).get_value());
        }
        else if (auto lee = dynamic_cast<ratio::core::enum_item *>(&left))
        {
            if (auto ree = dynamic_cast<ratio::core::enum_item *>(&right))
            { // we are comparing enums..
                auto r_vals = ov_th.value(ree->get_var());
                for (const auto &c_v : ov_th.value(lee->get_var()))
                    if (r_vals.count(c_v))
                        return true;
                return false;
            }
            else
                return ov_th.value(lee->get_var()).count(&right);
        }
        else if (dynamic_cast<ratio::core::enum_item *>(&right))
            return matches(right, left); // we swap, for simplifying code..
        else if (auto leci = dynamic_cast<ratio::core::atom *>(&left))
        {
            if (auto reci = dynamic_cast<ratio::core::atom *>(&right))
            { // we are comparing atoms..
                std::queue<ratio::core::type *> q;
                q.push(&left.get_type());
                while (!q.empty())
                {
                    for (const auto &[f_name, f] : q.front()->get_fields())
                        if (!f->is_synthetic())
                            if (!matches(*leci->get(f_name), *reci->get(f_name)))
                                return false;
                    for (const auto &stp : q.front()->get_supertypes())
                        q.push(stp);
                    q.pop();
                }
                return true;
            }
            else
                return false;
        }
        else
            return false;
    }

    ORATIOSOLVER_EXPORT void solver::new_disjunction(std::vector<std::unique_ptr<ratio::core::conjunction>> conjs)
    { // we create a new disjunction flaw..
        new_flaw(std::make_unique<disjunction_flaw>(*this, get_cause(), std::move(conjs)));
    }

    void solver::new_atom(ratio::core::expr &atm, const bool &is_fact)
    {
        // we create a new atom flaw..
        auto af = std::make_unique<atom_flaw>(*this, get_cause(), atm, is_fact);
        auto &c_af = *af;
        // we store some properties..
        atom_properties[&af->get_atom()] = {sat->new_var(), af.get()};

        // we store the flaw..
        new_flaw(std::move(af));

        // we check if we need to notify the new atom to any smart types..
        if (&atm->get_type().get_scope() != this)
        {
            std::queue<ratio::core::type *> q;
            q.push(static_cast<ratio::core::type *>(&atm->get_type().get_scope()));
            while (!q.empty())
            {
                if (auto smrtp = dynamic_cast<smart_type *>(q.front()))
                    smrtp->new_atom_flaw(c_af);
                for (const auto &stp : q.front()->get_supertypes())
                    q.push(stp);
                q.pop();
            }
        }
    }

    void solver::new_flaw(std::unique_ptr<flaw> f, const bool &enqueue)
    {
        if (std::any_of(f->get_causes().cbegin(), f->get_causes().cend(), [this](const auto &r)
                        { return sat->value(r->rho) == semitone::False; })) // there is no reason for introducing this flaw..
            return;
        // we initialize the flaw..
        f->init(); // flaws' initialization requires being at root-level..
        FIRE_NEW_FLAW(*f);

        if (enqueue) // we enqueue the flaw..
            gr->enqueue(*f);
        else // we directly expand the flaw..
            gr->expand_flaw(*f);

        switch (sat->value(f->phi))
        {
        case semitone::True: // we have a top-level (a landmark) flaw..
            if (enqueue || std::none_of(f->get_resolvers().cbegin(), f->get_resolvers().cend(), [this](const auto &r)
                                        { return sat->value(r->rho) == semitone::True; }))
                active_flaws.insert(f.get()); // the flaw has not yet already been solved (e.g. it has a single resolver)..
            break;
        case semitone::Undefined:   // we do not have a top-level (a landmark) flaw, nor an infeasible one..
            bind(variable(f->phi)); // we listen for the flaw to become active..
            break;
        }

        phis[variable(f->phi)].emplace_back(std::move(f));
    }

    void solver::new_resolver(std::unique_ptr<resolver> r)
    {
        FIRE_NEW_RESOLVER(*r);
        if (sat->value(r->rho) == semitone::Undefined) // we do not have a top-level (a landmark) resolver, nor an infeasible one..
            bind(variable(r->rho));                    // we listen for the resolver to become inactive..

        rhos[variable(r->rho)].push_back(std::move(r));
    }

    void solver::new_causal_link(flaw &f, resolver &r)
    {
        FIRE_CAUSAL_LINK_ADDED(f, r);
        r.preconditions.push_back(&f);
        f.supports.push_back(&r);
        // activating the resolver requires the activation of the flaw..
        [[maybe_unused]] bool new_clause = sat->new_clause({!r.rho, f.phi});
        assert(new_clause);
        // we introduce an ordering constraint..
        [[maybe_unused]] bool new_dist = sat->new_clause({!r.rho, idl_th.new_distance(r.effect.position, f.position, 0)});
        assert(new_dist);
    }

    void solver::expand_flaw(flaw &f)
    {
        assert(!f.expanded);

        // we expand the flaw..
        f.expand();

        // we apply the flaw's resolvers..
        for (const auto &r : f.resolvers)
            apply_resolver(*r);

        if (!sat->propagate())
            throw ratio::core::unsolvable_exception();

        // we clean up already solved flaws..
        if (sat->value(f.phi) == semitone::True && std::any_of(f.resolvers.cbegin(), f.resolvers.cend(), [this](const auto &r)
                                                               { return sat->value(r->rho) == semitone::True; }))
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
        catch (const ratio::core::inconsistency_exception &)
        { // the resolver is inapplicable..
            if (!sat->new_clause({!r.rho}))
                throw ratio::core::unsolvable_exception();
        }

        // we make some cleanings..
        restore_ni();
        res = nullptr;
    }

    void solver::set_cost(flaw &f, semitone::rational cost)
    {
        assert(f.est_cost != cost);
        if (!trail.empty()) // we store the current flaw's estimated cost, if not already stored, for allowing backtracking..
            trail.back().old_f_costs.try_emplace(&f, f.est_cost);

        // we update the flaw's estimated cost..
        f.est_cost = cost;
        FIRE_FLAW_COST_CHANGED(f);
    }

    ORATIOSOLVER_EXPORT void solver::assert_facts(std::vector<ratio::core::expr> facts)
    {
        for (const auto &f : facts)
            if (!sat->new_clause({!ni, static_cast<ratio::core::bool_item &>(*f).get_value()}))
                throw ratio::core::unsolvable_exception();
    }
    ORATIOSOLVER_EXPORT void solver::assert_facts(std::vector<semitone::lit> facts)
    {
        for (const auto &f : facts)
            if (!sat->new_clause({!ni, f}))
                throw ratio::core::unsolvable_exception();
    }

    ORATIOSOLVER_EXPORT bool solver::solve()
    {
        FIRE_STARTED_SOLVING();

        try
        {
            if (root_level())
            { // we make sure that gamma is at true..
                gr->build();
                gr->check();
            }
            assert(sat->value(gr->gamma) == semitone::True);

            // we search for a consistent solution without flaws..
#ifdef CHECK_INCONSISTENCIES
            // we solve all the current inconsistencies..
            solve_inconsistencies();

            while (!active_flaws.empty())
            {
                assert(std::all_of(active_flaws.cbegin(), active_flaws.cend(), [this](const auto &f)
                                   { return sat->value(f->phi) == semitone::True; })); // all the current flaws must be active..
                assert(std::all_of(active_flaws.cbegin(), active_flaws.cend(), [this](const auto &f)
                                   { return std::none_of(f->resolvers.cbegin(), f->resolvers.cend(), [this](resolver *r)
                                                         { return sat->value(r->rho) == semitone::True; }); })); // none of the current flaws must have already been solved..

                // this is the next flaw (i.e. the most expensive one) to be solved..
                auto best_flaw = std::min_element(active_flaws.cbegin(), active_flaws.cend(), [](const auto &f0, const auto &f1)
                                                  { return f0->get_estimated_cost() > f1->get_estimated_cost(); });
                assert(best_flaw != active_flaws.cend());
                FIRE_CURRENT_FLAW(**best_flaw);

                if (is_infinite((*best_flaw)->get_estimated_cost()))
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
                auto *best_res = (*best_flaw)->get_best_resolver();
                FIRE_CURRENT_RESOLVER(*best_res);

                assert(!is_infinite(best_res->get_estimated_cost()));

                // we apply the resolver..
                take_decision(best_res->get_rho());

                // we solve all the current inconsistencies..
                solve_inconsistencies();
            }
#else
            do
            {
                while (!active_flaws.empty())
                {
                    assert(std::all_of(active_flaws.cbegin(), active_flaws.cend(), [this](const auto f)
                                       { return sat->value(f->phi) == semitone::True; })); // all the current flaws must be active..
                    assert(std::all_of(active_flaws.cbegin(), active_flaws.cend(), [this](const auto f)
                                       { return std::none_of(f->resolvers.cbegin(), f->resolvers.cend(), [this](const auto r)
                                                             { return sat->value(r->rho) == semitone::True; }); })); // none of the current flaws must have already been solved..

                    // this is the next flaw (i.e. the most expensive one) to be solved..
                    auto best_flaw = std::min_element(active_flaws.cbegin(), active_flaws.cend(), [](const auto f0, const auto f1)
                                                      { return f0->get_estimated_cost() > f1->get_estimated_cost(); });
                    assert(best_flaw != active_flaws.cend());
                    FIRE_CURRENT_FLAW(**best_flaw);

                    if (is_infinite((*best_flaw)->get_estimated_cost()))
                    { // we don't know how to solve this flaw :(
                        do
                        { // we have to search..
                            next();
                        } while (std::any_of(active_flaws.cbegin(), active_flaws.cend(), [](const auto f)
                                             { return is_infinite(f->get_estimated_cost()); }));
                        continue;
                    }

                    // this is the next resolver (i.e. the cheapest one) to be applied..
                    auto *best_res = (*best_flaw)->get_best_resolver();
                    FIRE_CURRENT_RESOLVER(*best_res);

                    assert(!is_infinite(best_res->get_estimated_cost()));

                    // we apply the resolver..
                    take_decision(best_res->get_rho());
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
        catch (const ratio::core::unsolvable_exception &)
        { // the problem is unsolvable..
            FIRE_INCONSISTENT_PROBLEM();
            return false;
        }
    }

    ORATIOSOLVER_EXPORT void solver::take_decision(const semitone::lit &ch)
    {
        assert(sat->value(ch) == semitone::Undefined);

        // we take the decision..
        if (!sat->assume(ch))
            throw ratio::core::unsolvable_exception();

        if (root_level()) // we make sure that gamma is at true..
            gr->check();
        assert(sat->value(gr->gamma) == semitone::True);

        assert(std::all_of(phis.cbegin(), phis.cend(), [this](const auto &v_fs)
                           { return std::all_of(v_fs.second.cbegin(), v_fs.second.cend(), [this](const auto &f)
                                                { return (sat->value(f->phi) != semitone::False && f->get_estimated_cost() == (f->get_best_resolver() ? f->get_best_resolver()->get_estimated_cost() : semitone::rational::POSITIVE_INFINITY)) || is_positive_infinite(f->get_estimated_cost()); }); }));
        assert(std::all_of(rhos.cbegin(), rhos.cend(), [this](const auto &v_rs)
                           { return std::all_of(v_rs.second.cbegin(), v_rs.second.cend(), [this](const auto &r)
                                                { return is_positive_infinite(r->get_estimated_cost()) || sat->value(r->rho) != semitone::False; }); }));

        FIRE_STATE_CHANGED();
    }

    std::vector<std::unique_ptr<flaw>> solver::flush_pending_flaws() { return std::move(pending_flaws); }

    void solver::next()
    {
        LOG("next..");
        if (!sat->next())
            throw ratio::core::unsolvable_exception();

        if (root_level()) // we make sure that gamma is at true..
            gr->check();
        assert(sat->value(gr->gamma) == semitone::True);

        assert(std::all_of(phis.cbegin(), phis.cend(), [this](const auto &v_fs)
                           { return std::all_of(v_fs.second.cbegin(), v_fs.second.cend(), [this](const auto &f)
                                                { return (sat->value(f->phi) != semitone::False && f->get_estimated_cost() == (f->get_best_resolver() ? f->get_best_resolver()->get_estimated_cost() : semitone::rational::POSITIVE_INFINITY)) || is_positive_infinite(f->get_estimated_cost()); }); }));
        assert(std::all_of(rhos.cbegin(), rhos.cend(), [this](const auto &v_rs)
                           { return std::all_of(v_rs.second.cbegin(), v_rs.second.cend(), [this](const auto &r)
                                                { return is_positive_infinite(r->get_estimated_cost()) || sat->value(r->rho) != semitone::False; }); }));

        FIRE_STATE_CHANGED();
    }

    bool solver::propagate(const semitone::lit &p)
    {
        assert(cnfl.empty());
        assert(phis.count(variable(p)) || rhos.count(variable(p)));

        if (const auto at_phis_p = phis.find(variable(p)); at_phis_p != phis.cend())
            switch (sat->value(at_phis_p->first))
            {
            case semitone::True: // some flaws have been activated..
                for (const auto &f : at_phis_p->second)
                {
                    assert(!active_flaws.count(f.get()));
                    if (!root_level())
                        trail.back().new_flaws.insert(f.get());
                    if (std::none_of(f->resolvers.cbegin(), f->resolvers.cend(), [this](const auto r)
                                     { return sat->value(r->rho) == semitone::True; }))
                        active_flaws.insert(f.get()); // this flaw has been activated and not yet accidentally solved..
                    else if (!root_level())
                        trail.back().solved_flaws.insert(f.get()); // this flaw has been accidentally solved..
                    gr->activated_flaw(*f);
                }
                break;
            case semitone::False: // some flaws have been negated..
                for (const auto &f : at_phis_p->second)
                {
                    assert(!active_flaws.count(f.get()));
                    gr->negated_flaw(*f);
                }
                break;
            }

        if (const auto at_rhos_p = rhos.find(variable(p)); at_rhos_p != rhos.cend())
            switch (sat->value(at_rhos_p->first))
            {
            case semitone::True: // some resolvers have been activated..
                for (const auto &r : at_rhos_p->second)
                {
                    if (active_flaws.erase(&r->effect) && !root_level()) // this resolver has been activated, hence its effect flaw has been resolved (notice that we remove its effect only in case it was already active)..
                        trail.back().solved_flaws.insert(&r->effect);
                    gr->activated_resolver(*r);
                }
                break;
            case semitone::False: // some resolvers have been negated..
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
                           { return sat->value(f->phi) == semitone::True; }));
        assert(std::all_of(phis.cbegin(), phis.cend(), [this](const auto &v_fs)
                           { return std::all_of(v_fs.second.cbegin(), v_fs.second.cend(), [this](const auto &f)
                                                { return sat->value(f->phi) != semitone::False || is_positive_infinite(f->get_estimated_cost()); }); }));
        assert(std::all_of(rhos.cbegin(), rhos.cend(), [this](const auto &v_rs)
                           { return std::all_of(v_rs.second.cbegin(), v_rs.second.cend(), [this](const auto &r)
                                                { return sat->value(r->rho) != semitone::False || is_positive_infinite(r->get_estimated_cost()); }); }));
        return true;
    }

    void solver::push()
    {
        LOG(std::to_string(trail.size()) << " (" << std::to_string(active_flaws.size()) << ")");

        trail.emplace_back();
        gr->push();
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

        trail.pop_back();
        gr->pop();

        LOG(std::to_string(trail.size()) << " (" << std::to_string(active_flaws.size()) << ")");
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
                assert(sat->value(det_flw->front().first) != semitone::False);
                if (sat->value(det_flw->front().first) == semitone::Undefined)
                { // we can learn something from it..
                    std::vector<semitone::lit> learnt;
                    learnt.reserve(trail.size() + 1);
                    learnt.push_back(det_flw->front().first);
                    for (const auto &l : sat->get_decisions())
                        learnt.push_back(!l);
                    record(learnt);
                    if (!sat->propagate())
                        throw ratio::core::unsolvable_exception();

                    if (root_level()) // we make sure that gamma is at true..
                        gr->check();
                    assert(sat->value(gr->gamma) == semitone::True);
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
        // some cleanings..
        smart_types.clear();
        std::queue<ratio::core::type *> q;
        for ([[maybe_unused]] const auto &[tp_name, tp] : get_types())
            if (!tp->is_primitive())
                q.push(tp.get());
        while (!q.empty())
        {
            if (auto smrtp = dynamic_cast<smart_type *>(q.front()))
                smart_types.push_back(smrtp);
            for ([[maybe_unused]] const auto &[tp_name, tp] : q.front()->get_types())
                q.push(tp.get());
            q.pop();
        }
    }

    ORATIOSOLVER_EXPORT ratio::core::predicate &solver::get_impulse() const noexcept { return *imp_pred; }
    ORATIOSOLVER_EXPORT bool solver::is_impulse(const ratio::core::type &pred) const noexcept { return get_impulse().is_assignable_from(pred); }
    ORATIOSOLVER_EXPORT bool solver::is_impulse(const ratio::core::atom &atm) const noexcept { return is_impulse(atm.get_type()); }
    ORATIOSOLVER_EXPORT ratio::core::predicate &solver::get_interval() const noexcept { return *int_pred; }
    ORATIOSOLVER_EXPORT bool solver::is_interval(const ratio::core::type &pred) const noexcept { return get_interval().is_assignable_from(pred); }
    ORATIOSOLVER_EXPORT bool solver::is_interval(const ratio::core::atom &atm) const noexcept { return is_interval(atm.get_type()); }

    ORATIOSOLVER_EXPORT semitone::lbool solver::bool_value([[maybe_unused]] const ratio::core::bool_item &x) const noexcept { return sat->value(x.get_value()); }
    ORATIOSOLVER_EXPORT std::pair<semitone::inf_rational, semitone::inf_rational> solver::arith_bounds([[maybe_unused]] const ratio::core::arith_item &x) const noexcept
    {
        if (&x.get_type() == &get_time_type())
            return rdl_th.bounds(x.get_value());
        else
            return lra_th.bounds(x.get_value());
    }
    ORATIOSOLVER_EXPORT semitone::inf_rational solver::arith_value([[maybe_unused]] const ratio::core::arith_item &x) const noexcept
    {
        if (&x.get_type() == &get_time_type())
            return rdl_th.bounds(x.get_value()).first;
        else
            return lra_th.value(x.get_value());
    }
    ORATIOSOLVER_EXPORT std::unordered_set<semitone::var_value *> solver::enum_value([[maybe_unused]] const ratio::core::enum_item &x) const noexcept { return ov_th.value(x.get_var()); }

#ifdef BUILD_LISTENERS
    void solver::fire_new_flaw(const flaw &f) const
    {
        for (const auto &l : listeners)
            l->new_flaw(f);
    }
    void solver::fire_flaw_state_changed(const flaw &f) const
    {
        for (const auto &l : listeners)
            l->flaw_state_changed(f);
    }
    void solver::fire_flaw_cost_changed(const flaw &f) const
    {
        for (const auto &l : listeners)
            l->flaw_cost_changed(f);
    }
    void solver::fire_current_flaw(const flaw &f) const
    {
        for (const auto &l : listeners)
            l->current_flaw(f);
    }
    void solver::fire_new_resolver(const resolver &r) const
    {
        for (const auto &l : listeners)
            l->new_resolver(r);
    }
    void solver::fire_resolver_state_changed(const resolver &r) const
    {
        for (const auto &l : listeners)
            l->resolver_state_changed(r);
    }
    void solver::fire_current_resolver(const resolver &r) const
    {
        for (const auto &l : listeners)
            l->current_resolver(r);
    }
    void solver::fire_causal_link_added(const flaw &f, const resolver &r) const
    {
        for (const auto &l : listeners)
            l->causal_link_added(f, r);
    }
#endif

    ORATIOSOLVER_EXPORT json::json to_json(const solver &rhs) noexcept
    {
        std::set<ratio::core::item *> all_items;
        std::set<ratio::core::atom *> all_atoms;
        for ([[maybe_unused]] const auto &[pred_name, pred] : rhs.get_predicates())
            for (const auto &a : pred->get_instances())
                all_atoms.insert(static_cast<ratio::core::atom *>(&*a));
        std::queue<ratio::core::type *> q;
        for ([[maybe_unused]] const auto &[tp_name, tp] : rhs.get_types())
            if (!tp->is_primitive())
                q.push(tp.get());
        while (!q.empty())
        {
            for (const auto &i : q.front()->get_instances())
                all_items.insert(&*i);
            for ([[maybe_unused]] const auto &[pred_name, pred] : q.front()->get_predicates())
                for (const auto &a : pred->get_instances())
                    all_atoms.insert(static_cast<ratio::core::atom *>(&*a));
            for ([[maybe_unused]] const auto &[tp_name, tp] : q.front()->get_types())
                q.push(tp.get());
            q.pop();
        }

        json::json j_core;

        if (!all_items.empty())
        {
            json::array j_itms;
            for (const auto &itm : all_items)
                j_itms.push_back(to_json(*itm));
            j_core["items"] = std::move(j_itms);
        }

        if (!all_atoms.empty())
        {
            json::array j_atms;
            for (const auto &atm : all_atoms)
                j_atms.push_back(to_json(*atm));
            j_core["atoms"] = std::move(j_atms);
        }

        if (!rhs.get_vars().empty())
            j_core["exprs"] = to_json(rhs.get_vars());
        return j_core;
    }

    ORATIOSOLVER_EXPORT json::json to_timelines(solver &rhs) noexcept
    {
        json::array tls;

        // for each pulse, the atoms starting at that pulse..
        std::map<semitone::inf_rational, std::set<ratio::core::atom *>> starting_atoms;
        // all the pulses of the timeline..
        std::set<semitone::inf_rational> pulses;
        for ([[maybe_unused]] const auto &[p_name, p] : rhs.get_predicates())
            if (rhs.is_impulse(*p) || rhs.is_interval(*p))
                for (const auto &atm : p->get_instances())
                    if (&atm->get_type().get_core() != &rhs && rhs.get_sat_core()->value(get_sigma(rhs, static_cast<ratio::core::atom &>(*atm))) == semitone::True)
                    {
                        semitone::inf_rational start = rhs.get_core().arith_value(rhs.is_impulse(*p) ? static_cast<ratio::core::atom &>(*atm).get(RATIO_AT) : static_cast<ratio::core::atom &>(*atm).get(RATIO_START));
                        starting_atoms[start].insert(dynamic_cast<ratio::core::atom *>(&*atm));
                        pulses.insert(start);
                    }
        if (!starting_atoms.empty())
        {
            json::json slv_tl;
            slv_tl["id"] = reinterpret_cast<uintptr_t>(&rhs);
            slv_tl["name"] = "solver";
            json::array j_atms;
            for (const auto &p : pulses)
                for (const auto &atm : starting_atoms.at(p))
                    j_atms.push_back(to_json(*atm));
            slv_tl["values"] = std::move(j_atms);
            tls.push_back(std::move(slv_tl));
        }

        std::queue<ratio::core::type *> q;
        for ([[maybe_unused]] const auto &[tp_name, tp] : rhs.get_types())
            q.push(tp.get());

        while (!q.empty())
        {
            if (auto tl_tp = dynamic_cast<timeline *>(q.front()))
            {
                auto j_tls = tl_tp->extract();
                json::array &tls_array = j_tls;
                for (size_t i = 0; i < tls_array.size(); ++i)
                    tls.push_back(std::move(tls_array[i]));
            }
            for (const auto &[tp_name, st] : q.front()->get_types())
                q.push(st.get());
            q.pop();
        }

        return tls;
    }

    ORATIOSOLVER_EXPORT json::json to_json(const ratio::core::item &rhs) noexcept
    {
        solver &slv = static_cast<solver &>(rhs.get_type().get_core());
        json::json j_itm;
        j_itm["id"] = get_id(rhs);
        j_itm["type"] = rhs.get_type().get_full_name();
#ifdef COMPUTE_NAMES
        auto name = slv.guess_name(rhs);
        if (!name.empty())
            j_itm["name"] = name;
#endif
        if (auto ci = dynamic_cast<const ratio::core::complex_item *>(&rhs))
        {
            if (dynamic_cast<const ratio::core::enum_item *>(&rhs))
                j_itm["val"] = value(rhs);
            if (!ci->get_vars().empty())
                j_itm["exprs"] = to_json(ci->get_vars());
            if (auto atm = dynamic_cast<const ratio::core::atom *>(&rhs))
                j_itm["sigma"] = get_sigma(slv, *atm);
        }
        else
            j_itm["value"] = value(rhs);

        return j_itm;
    }
    ORATIOSOLVER_EXPORT json::json to_json(const std::map<std::string, ratio::core::expr> &vars) noexcept
    {
        json::array j_exprs;
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
    ORATIOSOLVER_EXPORT json::json value(const ratio::core::item &rhs) noexcept
    {
        solver &slv = static_cast<solver &>(rhs.get_type().get_core());
        if (&rhs.get_type() == &slv.get_bool_type())
        {
            const auto val = static_cast<const ratio::core::bool_item &>(rhs).get_value();

            json::json j_val;
            j_val["lit"] = (sign(val) ? "b" : "!b") + std::to_string(variable(val));
            switch (slv.get_sat_core()->value(val))
            {
            case semitone::True:
                j_val["val"] = "True";
                break;
            case semitone::False:
                j_val["val"] = "False";
                break;
            case semitone::Undefined:
                j_val["val"] = "Undefined";
                break;
            }
            return j_val;
        }
        else if (&rhs.get_type() == &slv.get_real_type())
        {
            const auto c_lin = static_cast<const ratio::core::arith_item &>(rhs).get_value();
            const auto [lb, ub] = slv.get_lra_theory().bounds(c_lin);
            const auto val = slv.get_lra_theory().value(c_lin);

            json::json j_val = to_json(val);
            j_val["lin"] = to_string(c_lin);
            if (!is_negative_infinite(lb))
                j_val["lb"] = to_json(lb);
            if (!is_positive_infinite(ub))
                j_val["ub"] = to_json(ub);
            return j_val;
        }
        else if (&rhs.get_type() == &slv.get_time_type())
        {
            const auto c_lin = static_cast<const ratio::core::arith_item &>(rhs).get_value();
            const auto [lb, ub] = slv.get_rdl_theory().bounds(c_lin);

            json::json j_val = to_json(lb);
            j_val["lin"] = to_string(c_lin);
            if (!is_negative_infinite(lb))
                j_val["lb"] = to_json(lb);
            if (!is_positive_infinite(ub))
                j_val["ub"] = to_json(ub);
            return j_val;
        }
        else if (&rhs.get_type() == &slv.get_string_type())
            return json::string_val(static_cast<const ratio::core::string_item &>(rhs).get_value());
        else if (auto ev = dynamic_cast<const ratio::core::enum_item *>(&rhs))
        {
            json::json j_val;
            j_val["var"] = std::to_string(ev->get_var());
            json::array vals;
            for (const auto &v : slv.get_ov_theory().value(ev->get_var()))
                vals.push_back(get_id(static_cast<ratio::core::item &>(*v)));
            j_val["vals"] = std::move(vals);
            return j_val;
        }
        else
            return get_id(rhs);
    }
} // namespace ratio::solver
