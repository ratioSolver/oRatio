#include "solver.h"
#include "atom.h"
#include "predicate.h"
#include "field.h"
#include <cassert>

namespace ratio::solver
{
    solver::solver() : sat_cr(), lra_th(sat_cr), ov_th(sat_cr), idl_th(sat_cr), rdl_th(sat_cr) {}

    ORATIO_EXPORT ratio::core::expr solver::new_bool() noexcept { return std::make_shared<ratio::core::bool_item>(get_bool_type(), semitone::lit(sat_cr.new_var())); }
    ORATIO_EXPORT ratio::core::expr solver::new_int() noexcept { return std::make_shared<ratio::core::arith_item>(get_int_type(), semitone::lin(lra_th.new_var(), semitone::rational::ONE)); }
    ORATIO_EXPORT ratio::core::expr solver::new_real() noexcept { return std::make_shared<ratio::core::arith_item>(get_real_type(), semitone::lin(lra_th.new_var(), semitone::rational::ONE)); }
    ORATIO_EXPORT ratio::core::expr solver::new_time_point() noexcept { return std::make_shared<ratio::core::arith_item>(get_time_type(), semitone::lin(lra_th.new_var(), semitone::rational::ONE)); }
    ORATIO_EXPORT ratio::core::expr solver::new_string() noexcept { return std::make_shared<ratio::core::string_item>(get_string_type(), ""); }

    ORATIO_EXPORT ratio::core::expr solver::new_enum(ratio::core::type &tp, const std::vector<ratio::core::expr> &allowed_vals)
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

        return std::make_shared<ratio::core::enum_item>(tp, ov_th.new_var(vals));
    }
    ORATIO_EXPORT ratio::core::expr solver::get(ratio::core::enum_item &var, const std::string &name)
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
            const auto var = sat_cr.new_disj(val.second);
            c_vars.push_back(var);
            c_vals.push_back(val.first);
            for (const auto &val_not : val_vars)
                if (val != val_not)
                    for (const auto &v : val_not.second)
                    {
                        [[maybe_unused]] bool nc = sat_cr.new_clause({!var, !v});
                        assert(nc);
                    }
        }

        if (&var.get_type() == &get_bool_type())
        {
            auto b = new_bool();
            [[maybe_unused]] bool nc;
            for (size_t i = 0; i < c_vars.size(); ++i)
            {
                nc = sat_cr.new_clause({!c_vars[i], sat_cr.new_eq(static_cast<ratio::core::bool_item &>(*c_vals[i]).get_value(), static_cast<ratio::core::bool_item &>(*b).get_value())});
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
                    nc = sat_cr.new_clause({!c_vars[i], lra_th.new_eq(static_cast<ratio::core::arith_item &>(*ie).get_value(), static_cast<ratio::core::arith_item &>(*c_vals[i]).get_value())});
                    assert(nc);
                }
                // we impose some bounds which might help propagation..
                nc = sat_cr.new_clause({lra_th.new_geq(static_cast<ratio::core::arith_item &>(*ie).get_value(), semitone::lin(min.get_rational()))});
                assert(nc);
                nc = sat_cr.new_clause({lra_th.new_leq(static_cast<ratio::core::arith_item &>(*ie).get_value(), semitone::lin(max.get_rational()))});
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
                    nc = sat_cr.new_clause({!c_vars[i], lra_th.new_eq(static_cast<ratio::core::arith_item &>(*re).get_value(), static_cast<ratio::core::arith_item &>(*c_vals[i]).get_value())});
                    assert(nc);
                }
                // we impose some bounds which might help propagation..
                nc = sat_cr.new_clause({lra_th.new_geq(static_cast<ratio::core::arith_item &>(*re).get_value(), semitone::lin(min.get_rational()))});
                assert(nc);
                nc = sat_cr.new_clause({lra_th.new_leq(static_cast<ratio::core::arith_item &>(*re).get_value(), semitone::lin(max.get_rational()))});
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
                    nc = sat_cr.new_clause({!c_vars[i], rdl_th.new_eq(static_cast<ratio::core::arith_item &>(*tm_pt).get_value(), static_cast<ratio::core::arith_item &>(*c_vals[i]).get_value())});
                    assert(nc);
                }
                // we impose some bounds which might help propagation..
                nc = sat_cr.new_clause({rdl_th.new_geq(static_cast<ratio::core::arith_item &>(*tm_pt).get_value(), semitone::lin(min.get_rational()))});
                assert(nc);
                nc = sat_cr.new_clause({rdl_th.new_leq(static_cast<ratio::core::arith_item &>(*tm_pt).get_value(), semitone::lin(max.get_rational()))});
                assert(nc);
                return tm_pt;
            }
        }
        else
            return std::make_shared<ratio::core::enum_item>(var.get_type().get_field(name).get_type(), ov_th.new_var(c_vars, c_vals));
    }
    ORATIO_EXPORT void solver::remove(ratio::core::expr &var, ratio::core::expr &val)
    {
        auto alw_var = ov_th.allows(static_cast<ratio::core::enum_item &>(*var).get_var(), *val);
        if (!sat_cr.new_clause({!ni, !alw_var}))
            throw ratio::core::unsolvable_exception();
    }

    ORATIO_EXPORT ratio::core::expr solver::negate(const ratio::core::expr &var) noexcept { return std::make_shared<ratio::core::bool_item>(get_bool_type(), !static_cast<ratio::core::bool_item &>(*var).get_value()); }

    ORATIO_EXPORT void solver::assert_facts(std::vector<ratio::core::expr> facts)
    {
        for (const auto &f : facts)
            if (!sat_cr.new_clause({!ni, static_cast<ratio::core::bool_item &>(*f).get_value()}))
                throw ratio::core::unsolvable_exception();
    }
    ORATIO_EXPORT void solver::assert_facts(std::vector<semitone::lit> facts)
    {
        for (const auto &f : facts)
            if (!sat_cr.new_clause({!ni, f}))
                throw ratio::core::unsolvable_exception();
    }
} // namespace ratio::solver
