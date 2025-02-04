#include "msatsolver.hpp"

namespace ratio
{
    msatsolver::msatsolver(std::string_view name) : graph(name)
    {
        cfg = msat_create_config();
        msat_set_option(cfg, "model_generation", "true");
        env = msat_create_env(cfg);
        read(INIT_STRING);
    }

    riddle::bool_expr msatsolver::new_bool() { return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), msat_make_constant(env, msat_declare_function(env, ("b" + std::to_string(bool_count++)).c_str(), msat_get_bool_type(env)))); }
    riddle::bool_expr msatsolver::new_bool(const bool value) { return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), value ? msat_make_true(env) : msat_make_false(env)); }
    utils::lbool msatsolver::bool_value(const riddle::bool_item &expr) const noexcept
    {
        auto val = msat_model_eval(env, mdl, static_cast<const bool_item &>(expr).get_expr());
        if (msat_term_is_true(env, val))
            return utils::True;
        else if (msat_term_is_false(env, val))
            return utils::False;
        else
            return utils::Undefined;
    }

    riddle::arith_expr msatsolver::new_int() { return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), msat_make_constant(env, msat_declare_function(env, ("i" + std::to_string(int_count++)).c_str(), msat_get_integer_type(env)))); }
    riddle::arith_expr msatsolver::new_int(const INT_TYPE value) { return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), msat_make_number(env, std::to_string(value).c_str())); }
    riddle::arith_expr msatsolver::new_int(const INT_TYPE lb, const INT_TYPE ub)
    {
        auto xpr = msat_make_constant(env, msat_declare_function(env, ("i" + std::to_string(int_count++)).c_str(), msat_get_integer_type(env)));
        msat_assert_formula(env, msat_make_geq(env, xpr, msat_make_number(env, std::to_string(lb).c_str())));
        msat_assert_formula(env, msat_make_leq(env, xpr, msat_make_number(env, std::to_string(ub).c_str())));
        return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(xpr));
    }
    riddle::arith_expr msatsolver::new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub)
    {
        auto xpr = msat_make_constant(env, msat_declare_function(env, ("i" + std::to_string(int_count++)).c_str(), msat_get_integer_type(env)));
        msat_assert_formula(env, msat_make_geq(env, xpr, msat_make_number(env, std::to_string(lb).c_str())));
        msat_assert_formula(env, msat_make_leq(env, xpr, msat_make_number(env, std::to_string(ub).c_str())));
        return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(xpr));
    }

    riddle::arith_expr msatsolver::new_real() { return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), msat_make_constant(env, msat_declare_function(env, ("r" + std::to_string(real_count++)).c_str(), msat_get_rational_type(env)))); }
    riddle::arith_expr msatsolver::new_real(utils::rational &&value) { return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), msat_make_number(env, to_string(value).c_str())); }
    riddle::arith_expr msatsolver::new_real(utils::rational &&lb, utils::rational &&ub)
    {
        auto xpr = msat_make_constant(env, msat_declare_function(env, ("r" + std::to_string(real_count++)).c_str(), msat_get_rational_type(env)));
        msat_assert_formula(env, msat_make_geq(env, xpr, msat_make_number(env, to_string(lb).c_str())));
        msat_assert_formula(env, msat_make_leq(env, xpr, msat_make_number(env, to_string(ub).c_str())));
        return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(xpr));
    }
    riddle::arith_expr msatsolver::new_uncertain_real(utils::rational &&lb, utils::rational &&ub)
    {
        auto xpr = msat_make_constant(env, msat_declare_function(env, ("r" + std::to_string(real_count++)).c_str(), msat_get_rational_type(env)));
        msat_assert_formula(env, msat_make_geq(env, xpr, msat_make_number(env, to_string(lb).c_str())));
        msat_assert_formula(env, msat_make_leq(env, xpr, msat_make_number(env, to_string(ub).c_str())));
        return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(xpr));
    }

    riddle::arith_expr msatsolver::new_time() { return utils::make_s_ptr<arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), msat_make_constant(env, msat_declare_function(env, ("t" + std::to_string(time_count++)).c_str(), msat_get_rational_type(env)))); }
    riddle::arith_expr msatsolver::new_time(utils::rational &&value) { return utils::make_s_ptr<arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), msat_make_number(env, to_string(value).c_str())); }

    utils::inf_rational msatsolver::arith_value(const riddle::arith_item &expr) const noexcept
    {
        auto val = msat_model_eval(env, mdl, static_cast<const arith_item &>(expr).get_expr());
        if (msat_term_is_number(env, val))
            return utils::inf_rational(val);
        else
            std::terminate();
    }

    riddle::string_expr msatsolver::new_string() { return utils::make_s_ptr<string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ""); }
    riddle::string_expr msatsolver::new_string(std::string &&value) { return utils::make_s_ptr<string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), value); }

    std::string msatsolver::string_value(const riddle::string_item &expr) const noexcept { return static_cast<const string_item &>(expr).get_expr(); }

    riddle::enum_expr msatsolver::new_enum(riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values)
    {
        assert(!values.empty());
        auto xpr = msat_make_constant(env, msat_declare_function(env, ("e" + std::to_string(enum_count++)).c_str(), msat_get_integer_type(env)));
        msat_assert_formula(env, msat_make_geq(env, xpr, msat_make_number(env, "0")));
        msat_assert_formula(env, msat_make_leq(env, xpr, msat_make_number(env, std::to_string(values.size()).c_str())));
        return utils::make_s_ptr<enum_item>(static_cast<riddle::enum_type &>(tp), std::move(xpr), std::move(values));
    }

    std::vector<utils::ref_wrapper<utils::enum_val>> msatsolver::enum_value(const riddle::enum_item &expr) const noexcept
    {
        auto val = msat_model_eval(env, mdl, static_cast<const enum_item &>(expr).get_expr());
        return {static_cast<utils::enum_val &>(*expr.get_values()[std::stoul(msat_term_repr(val))])};
    }

    riddle::bool_expr msatsolver::new_and(std::vector<riddle::bool_expr> &&exprs)
    {
        msat_term t0 = static_cast<const bool_item &>(*exprs[0]).get_expr();
        for (size_t i = 1; i < exprs.size(); i++)
            t0 = msat_make_and(env, t0, static_cast<const bool_item &>(*exprs[i]).get_expr());
        return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), t0);
    }

    riddle::bool_expr msatsolver::new_or(std::vector<riddle::bool_expr> &&exprs)
    {
        msat_term t0 = static_cast<const bool_item &>(*exprs[0]).get_expr();
        for (size_t i = 1; i < exprs.size(); i++)
            t0 = msat_make_or(env, t0, static_cast<const bool_item &>(*exprs[i]).get_expr());
        return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), t0);
    }

    riddle::bool_expr msatsolver::new_xor(std::vector<riddle::bool_expr> &&exprs)
    {
        msat_term t0 = static_cast<const bool_item &>(*exprs[0]).get_expr();
        for (size_t i = 1; i < exprs.size(); i++)
            t0 = msat_make_xor(env, t0, static_cast<const bool_item &>(*exprs[i]).get_expr());
        return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), t0);
    }
} // namespace ratio
