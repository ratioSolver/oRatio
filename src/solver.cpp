#include "solver.hpp"
#include "items.hpp"
#include "flaws.hpp"
#include <cassert>

namespace ratio
{
    solver::solver(std::string_view name) noexcept : riddle::core(name)
    {
        assigns[utils::FALSE_var] = utils::False; // the false constant..
    }

    riddle::bool_expr solver::new_bool()
    {
        const auto x = assigns.size();
        assigns.push_back(utils::Undefined);
        return std::make_shared<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), x);
    }
    riddle::bool_expr solver::new_bool(const bool value)
    {
        auto l = value ? utils::TRUE_lit : utils::FALSE_lit;
        return std::make_shared<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::move(l));
    }
    utils::lbool solver::bool_value(const riddle::bool_term &expr) const noexcept
    {
        return assigns.at(expr.get_id());
    }

    riddle::arith_expr solver::new_int() { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), lin_slv.new_var()); }
    riddle::arith_expr solver::new_int(const INT_TYPE value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::rational(value)); }
    riddle::arith_expr solver::new_int(const INT_TYPE lb, const INT_TYPE ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), lin_slv.new_var(utils::rational(lb), utils::rational(ub))); }
    riddle::arith_expr solver::new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), lin_slv.new_var(utils::rational(lb), utils::rational(ub))); }

    riddle::arith_expr solver::new_real() { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), lin_slv.new_var()); }
    riddle::arith_expr solver::new_real(utils::rational &&value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(value)); }
    riddle::arith_expr solver::new_real(utils::rational &&lb, utils::rational &&ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), lin_slv.new_var(std::move(lb), std::move(ub))); }
    riddle::arith_expr solver::new_uncertain_real(utils::rational &&lb, utils::rational &&ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), lin_slv.new_var(std::move(lb), std::move(ub))); }

    riddle::arith_expr solver::new_time() { return std::make_shared<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), lin_slv.new_var()); }
    riddle::arith_expr solver::new_time(utils::rational &&value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), std::move(value)); }

    utils::inf_rational solver::arith_value(const riddle::arith_term &expr) const noexcept { return lin_slv.val(static_cast<const riddle::arith_item &>(expr).get_lin()); }

    riddle::string_expr solver::new_string() { return std::make_shared<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ""); }
    riddle::string_expr solver::new_string(std::string &&value) { return std::make_shared<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), std::move(value)); }
    std::string solver::string_value(const riddle::string_term &expr) const noexcept { return static_cast<const riddle::string_item &>(expr).get_string(); }

    riddle::enum_expr solver::new_enum(riddle::component_type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values)
    {
        assert(!values.empty());
        std::vector<std::reference_wrapper<resolver>> causes;
        if (c_res)
            causes.push_back(c_res.value());

        std::vector<utils::lit> lits;
        if (values.size() == 1)
        { // if there is only one value, it must be true..
            lits.push_back(utils::TRUE_lit);
            return std::make_shared<riddle::enum_item>(tp, std::move(values), std::move(lits));
        }
        else
        { // otherwise, create a new variable for each value..
            for (size_t i = 0; i < values.size(); ++i)
                lits.push_back(mk_var());
            // .. and create a new enum flaw to manage the variable..
            auto &ef = new_flaw<enum_flaw>(*this, std::move(causes), std::make_shared<riddle::enum_item>(tp, std::move(values), std::move(lits)));
            return ef.get_var();
        }
    }
    std::vector<std::reference_wrapper<utils::enum_val>> solver::enum_value(const riddle::enum_term &expr) const noexcept
    {
        std::vector<std::reference_wrapper<utils::enum_val>> dom;
        for (const auto &val : static_cast<const riddle::enum_item &>(expr).get_values())
            if (value(static_cast<const riddle::enum_item &>(expr).get_lit(val.get())) != utils::False)
                dom.push_back(val.get());
        return dom;
    }

    utils::var solver::mk_var() noexcept
    {
        const auto x = assigns.size();
        assigns.push_back(utils::Undefined);
        return x;
    }
} // namespace ratio
