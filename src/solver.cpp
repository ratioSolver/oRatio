#include "solver.h"
#include "atom.h"
#include "predicate.h"
#include "field.h"
#include "conjunction.h"
#include "causal_graph.h"
#include "flaw.h"
#include "resolver.h"
#include <algorithm>
#include <cassert>

namespace ratio::solver
{
    ORATIO_EXPORT solver::solver() : solver(std::make_unique<causal_graph>()) {}
    ORATIO_EXPORT solver::solver(std::unique_ptr<causal_graph> gr) : sat_cr(), lra_th(sat_cr), ov_th(sat_cr), idl_th(sat_cr), rdl_th(sat_cr), gr(std::move(gr)) { gr->init(*this); }
    ORATIO_EXPORT solver::~solver() {}

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
    ORATIO_EXPORT ratio::core::expr solver::conj(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        std::vector<semitone::lit> lits;
        for (const auto &bex : exprs)
            lits.push_back(static_cast<ratio::core::bool_item &>(*bex).get_value());
        return std::make_shared<ratio::core::bool_item>(get_bool_type(), sat_cr.new_conj(std::move(lits)));
    }
    ORATIO_EXPORT ratio::core::expr solver::disj(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        std::vector<semitone::lit> lits;
        for (const auto &bex : exprs)
            lits.push_back(static_cast<ratio::core::bool_item &>(*bex).get_value());
        return std::make_shared<ratio::core::bool_item>(get_bool_type(), sat_cr.new_disj(std::move(lits)));
    }
    ORATIO_EXPORT ratio::core::expr solver::exct_one(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        std::vector<semitone::lit> lits;
        for (const auto &bex : exprs)
            lits.push_back(static_cast<ratio::core::bool_item &>(*bex).get_value());
        return std::make_shared<ratio::core::bool_item>(get_bool_type(), sat_cr.new_exct_one(std::move(lits)));
    }

    ORATIO_EXPORT ratio::core::expr solver::add(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        assert(exprs.size() > 1);
        semitone::lin l;
        for (const auto &aex : exprs)
            l += static_cast<ratio::core::arith_item &>(*aex).get_value();
        return std::make_shared<ratio::core::arith_item>(get_type(exprs), l);
    }
    ORATIO_EXPORT ratio::core::expr solver::sub(const std::vector<ratio::core::expr> &exprs) noexcept
    {
        assert(exprs.size() > 1);
        semitone::lin l;
        for (auto it = exprs.cbegin(); it != exprs.cend(); ++it)
            if (it == exprs.cbegin())
                l += static_cast<ratio::core::arith_item &>(**it).get_value();
            else
                l -= static_cast<ratio::core::arith_item &>(**it).get_value();
        return std::make_shared<ratio::core::arith_item>(get_type(exprs), l);
    }
    ORATIO_EXPORT ratio::core::expr solver::mult(const std::vector<ratio::core::expr> &exprs) noexcept
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
            for (auto it = ++exprs.cbegin(); it != exprs.cend(); ++it)
            {
                assert(lra_th.value(static_cast<ratio::core::arith_item &>(**it).get_value()).get_infinitesimal() == semitone::rational::ZERO);
                l *= lra_th.value(static_cast<ratio::core::arith_item &>(**it).get_value()).get_rational();
            }
            return std::make_shared<ratio::core::arith_item>(get_type(exprs), l);
        }
    }
    ORATIO_EXPORT ratio::core::expr solver::div(const std::vector<ratio::core::expr> &exprs) noexcept
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
    ORATIO_EXPORT ratio::core::expr solver::minus(const ratio::core::expr &ex) noexcept { return std::make_shared<ratio::core::arith_item>(ex->get_type(), -static_cast<ratio::core::arith_item &>(*ex).get_value()); }

    ORATIO_EXPORT ratio::core::expr solver::lt(const ratio::core::expr &left, const ratio::core::expr &right) noexcept
    {
        if (&get_type({left, right}) == &get_time_type())
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), rdl_th.new_lt(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
        else
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), lra_th.new_lt(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
    }
    ORATIO_EXPORT ratio::core::expr solver::leq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept
    {
        if (&get_type({left, right}) == &get_time_type())
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), rdl_th.new_leq(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
        else
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), lra_th.new_leq(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
    }
    ORATIO_EXPORT ratio::core::expr solver::eq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept
    {
        if (left == right) // the two items are the same item..
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), semitone::TRUE_lit);
        else if (&left->get_type() == &get_bool_type() && &right->get_type() == &get_bool_type()) // we are comparing boolean expressions..
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), sat_cr.new_eq(static_cast<ratio::core::bool_item &>(*left).get_value(), static_cast<ratio::core::bool_item &>(*right).get_value()));
        else if (&left->get_type() == &get_string_type() && &right->get_type() == &get_string_type()) // we are comparing string expressions..
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), static_cast<ratio::core::string_item &>(*left).get_value() == static_cast<ratio::core::string_item &>(*right).get_value() ? semitone::TRUE_lit : semitone::FALSE_lit);
        else if ((&left->get_type() == &get_int_type() || &left->get_type() == &get_real_type() || &left->get_type() == &get_time_type()) && (&right->get_type() == &get_int_type() || &right->get_type() == &get_real_type() || &right->get_type() == &get_time_type()))
        { // we are comparing arithmetic expressions..
            if (&get_type({left, right}) == &get_time_type())
                return std::make_shared<ratio::core::bool_item>(get_bool_type(), rdl_th.new_eq(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
            else
                return std::make_shared<ratio::core::bool_item>(get_bool_type(), lra_th.new_eq(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
        }
        else if (dynamic_cast<ratio::core::enum_item *>(right.get()))
            return eq(right, left); // we swap, for simplifying code..
        else if (ratio::core::enum_item *le = dynamic_cast<ratio::core::enum_item *>(left.get()))
        { // we are comparing enums..
            if (ratio::core::enum_item *re = dynamic_cast<ratio::core::enum_item *>(right.get()))
                return std::make_shared<ratio::core::bool_item>(get_bool_type(), ov_th.new_eq(le->get_var(), re->get_var()));
            else
                return std::make_shared<ratio::core::bool_item>(get_bool_type(), ov_th.allows(le->get_var(), *left));
        }
        else
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), semitone::FALSE_lit);
    }
    ORATIO_EXPORT ratio::core::expr solver::geq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept
    {
        if (&get_type({left, right}) == &get_time_type())
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), rdl_th.new_gt(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
        else
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), lra_th.new_gt(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
    }
    ORATIO_EXPORT ratio::core::expr solver::gt(const ratio::core::expr &left, const ratio::core::expr &right) noexcept
    {
        if (&get_type({left, right}) == &get_time_type())
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), rdl_th.new_geq(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
        else
            return std::make_shared<ratio::core::bool_item>(get_bool_type(), lra_th.new_geq(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value()));
    }

    ORATIO_EXPORT bool solver::matches(const ratio::core::expr &left, const ratio::core::expr &right) noexcept
    {
        if (left == right) // the two items are the same item..
            return true;
        else if (&left->get_type() == &get_bool_type() && &right->get_type() == &get_bool_type())
        { // we are comparing boolean expressions..
            auto l_val = sat_cr.value(static_cast<ratio::core::bool_item &>(*left).get_value());
            auto r_val = sat_cr.value(static_cast<ratio::core::bool_item &>(*right).get_value());
            return l_val == r_val || l_val == semitone::Undefined || r_val == semitone::Undefined;
        }
        else if (&left->get_type() == &get_string_type() && &right->get_type() == &get_string_type()) // we are comparing string expressions..
            return static_cast<ratio::core::string_item &>(*left).get_value() == static_cast<ratio::core::string_item &>(*right).get_value();
        else if ((&left->get_type() == &get_int_type() || &left->get_type() == &get_real_type() || &left->get_type() == &get_time_type()) && (&right->get_type() == &get_int_type() || &right->get_type() == &get_real_type() || &right->get_type() == &get_time_type()))
        { // we are comparing arithmetic expressions..
            if (&get_type({left, right}) == &get_time_type())
                return rdl_th.matches(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value());
            else
                return lra_th.matches(static_cast<ratio::core::arith_item &>(*left).get_value(), static_cast<ratio::core::arith_item &>(*right).get_value());
        }
        else if (dynamic_cast<ratio::core::enum_item *>(right.get()))
            return matches(right, left); // we swap, for simplifying code..
        else if (ratio::core::enum_item *le = dynamic_cast<ratio::core::enum_item *>(left.get()))
        { // we are comparing enums..
            if (ratio::core::enum_item *re = dynamic_cast<ratio::core::enum_item *>(right.get()))
            { // the right expression is an enum..
                auto r_vals = ov_th.value(re->get_var());
                for (const auto &c_v : ov_th.value(le->get_var()))
                    if (r_vals.count(c_v))
                        return true;
                return false;
            }
            else
                return ov_th.value(le->get_var()).count(right.get());
        }
        else
            return false;
    }

    ORATIO_EXPORT void solver::new_disjunction(std::vector<std::unique_ptr<ratio::core::conjunction>> conjs)
    {
    }

    void solver::new_atom(ratio::core::atom &atm, const bool &is_fact)
    {
    }

    void solver::new_flaw(std::unique_ptr<flaw> f, const bool &enqueue)
    {
        if (std::any_of(f->get_causes().cbegin(), f->get_causes().cend(), [this](const auto &r)
                        { return sat_cr.value(r->get_rho()) == semitone::False; })) // there is no reason for introducing this flaw..
            return;
        // we initialize the flaw..
        f->init(); // flaws' initialization requires being at root-level..
        FIRE_NEW_FLAW(f);

        if (enqueue) // we enqueue the flaw..
            gr->enqueue(*f);
        else // we directly expand the flaw..
            gr->expand_flaw(*f);

        switch (sat_cr.value(f->get_phi()))
        {
        case semitone::True: // we have a top-level (a landmark) flaw..
            if (enqueue || std::none_of(f->get_resolvers().cbegin(), f->get_resolvers().cend(), [this](const auto &r)
                                        { return sat_cr.value(r->get_rho()) == semitone::True; }))
                active_flaws.insert(f.get()); // the flaw has not yet already been solved (e.g. it has a single resolver)..
            break;
        case semitone::Undefined: // we listen for the flaw to become active..
            // phis[semitone::variable(f->get_phi())].push_back(std::move(f));
            // bind(semitone::variable(f->get_phi()));
            break;
        }
    }

    void solver::new_resolver(std::unique_ptr<resolver> r)
    {
        FIRE_NEW_RESOLVER(r);
        if (sat_cr.value(r->get_rho()) == semitone::Undefined) // we do not have a top-level (a landmark) resolver, nor an infeasible one..
        {
            // we listen for the resolver to become inactive..
            // rhos[semitone::variable(r->get_rho())].push_back(std::move(r));
            // bind(semitone::variable(r->get_rho()));
        }
    }

    void solver::new_causal_link(flaw &f, resolver &r) {}

    void solver::expand_flaw(flaw &f) {}
    void solver::apply_resolver(resolver &r) {}
    void solver::set_cost(flaw &f, semitone::rational cost) {}

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

    ORATIO_EXPORT bool solver::solve() { return true; }
    ORATIO_EXPORT void solver::take_decision(const semitone::lit &ch) {}
} // namespace ratio::solver
