#include <cassert>
#include <algorithm>
#include "solver.hpp"
#include "init.hpp"
#include "sat_core.hpp"
#include "logging.hpp"

namespace ratio
{
    solver::solver(const std::string &name, bool i) noexcept : theory(std::make_shared<semitone::sat_core>()), name(name), lra(get_sat_ptr()), idl(get_sat_ptr()), rdl(get_sat_ptr()), ov(get_sat_ptr()), gr(*this)
    {
        if (i)
            init();
    }

    void solver::init() noexcept
    {
        LOG_INFO("Initializing solver " << name);
    }

    std::shared_ptr<riddle::item> solver::new_bool() noexcept { return std::make_shared<bool_item>(get_bool_type(), utils::lit(sat->new_var())); }
    std::shared_ptr<riddle::item> solver::new_bool(bool value) noexcept { return std::make_shared<bool_item>(get_bool_type(), value ? utils::TRUE_lit : utils::FALSE_lit); }
    std::shared_ptr<riddle::item> solver::new_int() noexcept { return std::make_shared<arith_item>(get_int_type(), utils::lin(lra.new_var(), utils::rational::one)); }
    std::shared_ptr<riddle::item> solver::new_int(INTEGER_TYPE value) noexcept { return std::make_shared<arith_item>(get_int_type(), utils::lin(utils::rational(value))); }
    std::shared_ptr<riddle::item> solver::new_real() noexcept { return std::make_shared<arith_item>(get_real_type(), utils::lin(lra.new_var(), utils::rational::one)); }
    std::shared_ptr<riddle::item> solver::new_real(const utils::rational &value) noexcept { return std::make_shared<arith_item>(get_real_type(), utils::lin(value)); }
    std::shared_ptr<riddle::item> solver::new_time() noexcept { return std::make_shared<arith_item>(get_time_type(), utils::lin(rdl.new_var(), utils::rational::one)); }
    std::shared_ptr<riddle::item> solver::new_time(const utils::rational &value) noexcept { return std::make_shared<arith_item>(get_time_type(), utils::lin(value)); }
    std::shared_ptr<riddle::item> solver::new_string() noexcept { return std::make_shared<string_item>(get_string_type()); }
    std::shared_ptr<riddle::item> solver::new_string(const std::string &value) noexcept { return std::make_shared<string_item>(get_string_type(), value); }
    std::shared_ptr<riddle::item> solver::new_enum(riddle::type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values) { return std::make_shared<enum_item>(tp, ov.new_var(std::move(values))); }

    std::shared_ptr<riddle::item> solver::minus(const std::shared_ptr<riddle::item> &xpr)
    {
        assert(is_arith(*xpr));
        if (is_int(*xpr))
            return std::make_shared<arith_item>(get_int_type(), -std::static_pointer_cast<arith_item>(xpr)->get_value());
        else if (is_real(*xpr))
            return std::make_shared<arith_item>(get_real_type(), -std::static_pointer_cast<arith_item>(xpr)->get_value());
        assert(is_time(*xpr));
        return std::make_shared<arith_item>(get_time_type(), -std::static_pointer_cast<arith_item>(xpr)->get_value());
    }

    std::shared_ptr<riddle::item> solver::add(const std::vector<std::shared_ptr<riddle::item>> &xprs)
    {
        assert(xprs.size() > 1);
        utils::lin sum;
        for (const auto &xpr : xprs)
        {
            assert(is_arith(*xpr));
            sum = sum + std::static_pointer_cast<arith_item>(xpr)->get_value();
        }
        auto &tp = determine_type(xprs);
        if (&tp.get_scope().get_core().get_int_type() == &tp)
            return std::make_shared<arith_item>(static_cast<riddle::int_type &>(tp), sum);
        assert(&tp.get_scope().get_core().get_real_type() == &tp);
        return std::make_shared<arith_item>(static_cast<riddle::real_type &>(tp), sum);
    }

    std::shared_ptr<riddle::item> solver::sub(const std::vector<std::shared_ptr<riddle::item>> &xprs)
    {
        assert(xprs.size() > 1);
        utils::lin diff = std::static_pointer_cast<arith_item>(xprs[0])->get_value();
        for (size_t i = 1; i < xprs.size(); i++)
        {
            assert(is_arith(*xprs[i]));
            diff = diff - std::static_pointer_cast<arith_item>(xprs[i])->get_value();
        }
        auto &tp = determine_type(xprs);
        if (&tp.get_scope().get_core().get_int_type() == &tp)
            return std::make_shared<arith_item>(static_cast<riddle::int_type &>(tp), diff);
        else if (&tp.get_scope().get_core().get_real_type() == &tp)
            return std::make_shared<arith_item>(static_cast<riddle::real_type &>(tp), diff);
        assert(&tp.get_scope().get_core().get_time_type() == &tp);
        return std::make_shared<arith_item>(static_cast<riddle::time_type &>(tp), diff);
    }

    std::shared_ptr<riddle::item> solver::mul(const std::vector<std::shared_ptr<riddle::item>> &xprs)
    {
        assert(xprs.size() > 1);
        if (auto var_it = std::find_if(xprs.begin(), xprs.end(), [this](const auto &x)
                                       { return !is_constant(*x); });
            var_it != xprs.end())
        {
            utils::lin prod = std::static_pointer_cast<arith_item>(*var_it)->get_value();
            for (const auto &xpr : xprs)
                if (xpr != *var_it)
                {
                    assert(is_arith(*xpr));
                    assert(is_constant(*xpr) && xpr->get_type().get_scope().get_core().arithmetic_value(*xpr).get_infinitesimal() == 0);
                    prod = prod * xpr->get_type().get_scope().get_core().arithmetic_value(*xpr).get_rational();
                }
            auto &tp = determine_type(xprs);
            if (&tp.get_scope().get_core().get_int_type() == &tp)
                return std::make_shared<arith_item>(static_cast<riddle::int_type &>(tp), prod);
            assert(&tp.get_scope().get_core().get_real_type() == &tp);
            return std::make_shared<arith_item>(static_cast<riddle::real_type &>(tp), prod);
        }
        else
        {
            assert(std::all_of(xprs.begin(), xprs.end(), [this](const auto &x)
                               { return is_arith(*x) && is_constant(*x) && x->get_type().get_scope().get_core().arithmetic_value(*x).get_infinitesimal() == 0; }));
            utils::rational prod = xprs[0]->get_type().get_scope().get_core().arithmetic_value(*xprs[0]).get_rational();
            for (size_t i = 1; i < xprs.size(); i++)
                prod = prod * xprs[i]->get_type().get_scope().get_core().arithmetic_value(*xprs[i]).get_rational();
            auto &tp = determine_type(xprs);
            if (&tp.get_scope().get_core().get_int_type() == &tp)
                return std::make_shared<arith_item>(static_cast<riddle::int_type &>(tp), utils::lin(prod));
            assert(&tp.get_scope().get_core().get_real_type() == &tp);
            return std::make_shared<arith_item>(static_cast<riddle::real_type &>(tp), utils::lin(prod));
        }
    }

    std::shared_ptr<riddle::item> solver::div(const std::vector<std::shared_ptr<riddle::item>> &xprs)
    {
        assert(xprs.size() > 1);
        utils::lin quot = std::static_pointer_cast<arith_item>(xprs[0])->get_value();
        for (size_t i = 1; i < xprs.size(); i++)
        {
            assert(is_arith(*xprs[i]));
            assert(is_constant(*xprs[i]) && xprs[i]->get_type().get_scope().get_core().arithmetic_value(*xprs[i]).get_infinitesimal() == 0);
            quot = quot / xprs[i]->get_type().get_scope().get_core().arithmetic_value(*xprs[i]).get_rational();
        }
        auto &tp = determine_type(xprs);
        if (&tp.get_scope().get_core().get_int_type() == &tp)
            return std::make_shared<arith_item>(static_cast<riddle::int_type &>(tp), quot);
        assert(&tp.get_scope().get_core().get_real_type() == &tp);
        return std::make_shared<arith_item>(static_cast<riddle::real_type &>(tp), quot);
    }

    std::shared_ptr<riddle::item> solver::lt(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) { return std::make_shared<bool_item>(get_bool_type(), lra.new_lt(std::static_pointer_cast<arith_item>(lhs)->get_value(), std::static_pointer_cast<arith_item>(rhs)->get_value())); }
    std::shared_ptr<riddle::item> solver::leq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) { return std::make_shared<bool_item>(get_bool_type(), lra.new_leq(std::static_pointer_cast<arith_item>(lhs)->get_value(), std::static_pointer_cast<arith_item>(rhs)->get_value())); }
    std::shared_ptr<riddle::item> solver::gt(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) { return std::make_shared<bool_item>(get_bool_type(), lra.new_lt(std::static_pointer_cast<arith_item>(rhs)->get_value(), std::static_pointer_cast<arith_item>(lhs)->get_value())); }
    std::shared_ptr<riddle::item> solver::geq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) { return std::make_shared<bool_item>(get_bool_type(), lra.new_leq(std::static_pointer_cast<arith_item>(rhs)->get_value(), std::static_pointer_cast<arith_item>(lhs)->get_value())); }

    std::shared_ptr<riddle::item> solver::eq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) { return nullptr; }

    std::shared_ptr<riddle::item> solver::conj(const std::vector<std::shared_ptr<riddle::item>> &exprs)
    {
        std::vector<utils::lit> lits;
        lits.reserve(exprs.size());
        for (const auto &xpr : exprs)
            lits.push_back(std::static_pointer_cast<bool_item>(xpr)->get_value());
        return std::make_shared<bool_item>(get_bool_type(), sat->new_conj(std::move(lits)));
    }
    std::shared_ptr<riddle::item> solver::disj(const std::vector<std::shared_ptr<riddle::item>> &exprs)
    {
        std::vector<utils::lit> lits;
        lits.reserve(exprs.size());
        for (const auto &xpr : exprs)
            lits.push_back(std::static_pointer_cast<bool_item>(xpr)->get_value());
        return std::make_shared<bool_item>(get_bool_type(), sat->new_disj(std::move(lits)));
    }
    std::shared_ptr<riddle::item> solver::exct_one(const std::vector<std::shared_ptr<riddle::item>> &exprs)
    {
        std::vector<utils::lit> lits;
        lits.reserve(exprs.size());
        for (const auto &xpr : exprs)
            lits.push_back(std::static_pointer_cast<bool_item>(xpr)->get_value());
        return nullptr;
    }
    std::shared_ptr<riddle::item> solver::negate(const std::shared_ptr<riddle::item> &expr) { return std::make_shared<bool_item>(get_bool_type(), !std::static_pointer_cast<bool_item>(expr)->get_value()); }

    void solver::assert_fact(const std::shared_ptr<riddle::item> &fact)
    {
        assert(&fact->get_type() == &get_bool_type()); // the expression must be a boolean..
        if (!sat->new_clause({res.has_value() ? !res.value().get_rho() : utils::FALSE_lit, std::static_pointer_cast<bool_item>(fact)->get_value()}))
            throw std::runtime_error("Unsatisfiable clause");
    }
} // namespace ratio