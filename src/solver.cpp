#include "solver.h"
#include "items.h"
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
            // TODO: create a new flaw for the enum expression..
            return enum_expr;
        }
        }
    }
} // namespace ratio
