#include "solver.h"
#include "items.h"
#include "enum_flaw.h"
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
                    nc = sat->new_clause({!c_vars[i], sat->new_eq(static_cast<ratio::bool_item &>(*c_vals[i]).get_lit(), static_cast<ratio::bool_item &>(*b_xpr).get_lit())});
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
                    const auto [lb, ub] = lra_th.bounds(static_cast<ratio::arith_item *>(val)->get_lin());
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
                        nc = sat->new_clause({!c_vars[i], lra_th.new_eq(static_cast<ratio::arith_item &>(*arith_xpr).get_lin(), static_cast<ratio::arith_item *>(c_vals[i])->get_lin())});
                        assert(nc);
                    }
                    // we try to impose some bounds which might help propagation..
                    if (min.get_infinitesimal() == utils::rational::ZERO && min.get_rational() > utils::rational::NEGATIVE_INFINITY)
                    {
                        nc = sat->new_clause({lra_th.new_geq(static_cast<ratio::arith_item &>(*arith_xpr).get_lin(), semitone::lin(min.get_rational()))});
                        assert(nc);
                    }
                    if (max.get_infinitesimal() == utils::rational::ZERO && max.get_rational() < utils::rational::POSITIVE_INFINITY)
                    {
                        nc = sat->new_clause({lra_th.new_leq(static_cast<ratio::arith_item &>(*arith_xpr).get_lin(), semitone::lin(max.get_rational()))});
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
                    const auto [lb, ub] = rdl_th.bounds(static_cast<ratio::arith_item *>(val)->get_lin());
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
                        nc = sat->new_clause({!c_vars[i], rdl_th.new_eq(static_cast<ratio::arith_item &>(*tp_xpr).get_lin(), static_cast<ratio::arith_item *>(c_vals[i])->get_lin())});
                        assert(nc);
                    }
                    // we try to impose some bounds which might help propagation..
                    if (min.get_infinitesimal() == utils::rational::ZERO && min.get_rational() > utils::rational::NEGATIVE_INFINITY)
                    {
                        nc = sat->new_clause({rdl_th.new_geq(static_cast<ratio::arith_item &>(*tp_xpr).get_lin(), semitone::lin(min.get_rational()))});
                        assert(nc);
                    }
                    if (max.get_infinitesimal() == utils::rational::ZERO && max.get_rational() < utils::rational::POSITIVE_INFINITY)
                    {
                        nc = sat->new_clause({rdl_th.new_leq(static_cast<ratio::arith_item &>(*tp_xpr).get_lin(), semitone::lin(max.get_rational()))});
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
        else if (auto ev = dynamic_cast<const ratio::enum_item *>(&itm))
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
