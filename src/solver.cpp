#include "solver.h"
#include "items.h"
#include "enum_flaw.h"
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

    riddle::expr solver::new_item(riddle::complex_type &tp) { return new complex_item(tp); }

    riddle::expr solver::new_enum(riddle::type &tp, const std::vector<riddle::expr> &xprs)
    {
        assert(tp != get_bool_type());
        assert(tp != get_int_type());
        assert(tp != get_real_type());
        assert(tp != get_time_type());
        assert(tp != get_string_type());
        switch (xprs.size())
        {
        case 0:
            throw riddle::inconsistency_exception();
        case 1:
            return xprs[0];
        default:
        {
            std::vector<semitone::var_value *> vals;
            vals.reserve(xprs.size());
            for (auto &xpr : xprs)
                if (auto vv = dynamic_cast<semitone::var_value *>(&*xpr))
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
                vals.push_back(get_id(static_cast<complex_item &>(*v)));
            j_val["vals"] = std::move(vals);
            return j_val;
        }
        else
            return get_id(itm);
    }
} // namespace ratio
