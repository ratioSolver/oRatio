#include "stsolver.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio
{
    stsolver::stsolver(std::string_view name) noexcept : graph(name) {}

    riddle::bool_expr stsolver::new_bool() { return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), utils::lit(net.new_var())); }
    riddle::bool_expr stsolver::new_bool(const bool value) { return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), value ? utils::TRUE_lit : utils::FALSE_lit); }
    utils::lbool stsolver::bool_value(const riddle::bool_item &expr) const noexcept { return net.value(static_cast<const bool_item &>(expr).get_expr()); }

    riddle::arith_expr stsolver::new_int() { return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(net.new_int(), utils::rational::one)); }
    riddle::arith_expr stsolver::new_int(const INT_TYPE value) { return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(utils::rational(value))); }
    riddle::arith_expr stsolver::new_int(const INT_TYPE lb, const INT_TYPE ub) { return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(net.new_int(utils::rational(lb), utils::rational(ub)), utils::rational::one)); }
    riddle::arith_expr stsolver::new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub) { return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(net.new_int(utils::rational(lb), utils::rational(ub)), utils::rational::one)); }

    riddle::arith_expr stsolver::new_real() { return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(net.new_real(), utils::rational::one)); }
    riddle::arith_expr stsolver::new_real(utils::rational &&value) { return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(std::move(value))); }
    riddle::arith_expr stsolver::new_real(utils::rational &&lb, utils::rational &&ub) { return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(net.new_real(std::move(lb), std::move(ub)), utils::rational::one)); }
    riddle::arith_expr stsolver::new_uncertain_real(utils::rational &&lb, utils::rational &&ub) { return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(net.new_real(std::move(lb), std::move(ub)), utils::rational::one)); }

    riddle::arith_expr stsolver::new_time() { return utils::make_s_ptr<arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), utils::lin(net.new_tp(), utils::rational::one)); }
    riddle::arith_expr stsolver::new_time(utils::rational &&value) { return utils::make_s_ptr<arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), utils::lin(std::move(value))); }

    utils::inf_rational stsolver::arith_value(const riddle::arith_item &expr) const noexcept
    {
        if (expr.get_type().get_name() == riddle::int_kw || expr.get_type().get_name() == riddle::real_kw)
            return net.arith_value(static_cast<const arith_item &>(expr).get_expr());
        else
            return utils::inf_rational(net.tp_bounds(static_cast<const arith_item &>(expr).get_expr().vars.begin()->first).first);
    }

    riddle::string_expr stsolver::new_string() { return utils::make_s_ptr<string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ""); }
    riddle::string_expr stsolver::new_string(std::string &&value) { return utils::make_s_ptr<string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), std::move(value)); }
    std::string stsolver::string_value(const riddle::string_item &expr) const noexcept { return static_cast<const string_item &>(expr).get_expr(); }

    riddle::enum_expr stsolver::new_enum(riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values)
    {
        assert(!values.empty());
        return utils::make_s_ptr<enum_item>(static_cast<riddle::enum_type &>(tp), net.new_int(utils::rational::zero, utils::rational(values.size() - 1)), std::move(values));
    }
    std::vector<utils::ref_wrapper<utils::enum_val>> stsolver::enum_value(const riddle::enum_item &expr) const noexcept { return {static_cast<utils::enum_val &>(*expr.get_values()[net.arith_value(static_cast<const enum_item &>(expr).get_expr()).get_rational().numerator()])}; }
} // namespace ratio