#include "stsolver.hpp"
#include "stflaws.hpp"
#include "sttypes.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio
{
    bool_item::bool_item(riddle::bool_type &tp, utils::lit &&expr) noexcept : riddle::bool_item(tp, std::move(expr)) {}
    riddle::bool_expr bool_item::operator==(riddle::expr rhs) const {}

    arith_item::arith_item(riddle::int_type &tp, utils::lin &&expr) noexcept : riddle::arith_item(tp, std::move(expr)) {}
    arith_item::arith_item(riddle::real_type &tp, utils::lin &&expr) noexcept : riddle::arith_item(tp, std::move(expr)) {}
    arith_item::arith_item(riddle::time_type &tp, utils::lin &&expr) noexcept : riddle::arith_item(tp, std::move(expr)) {}
    riddle::bool_expr arith_item::operator==(riddle::expr rhs) const {}

    string_item::string_item(riddle::string_type &tp, std::string &&expr) noexcept : riddle::string_item(tp, std::move(expr)) {}
    riddle::bool_expr string_item::operator==(riddle::expr rhs) const {}

    enum_item::enum_item(riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values, utils::var &&expr) noexcept : riddle::enum_item(tp, std::move(values), std::move(expr)) {}
    riddle::bool_expr enum_item::operator==(riddle::expr rhs) const {}

    atom::atom(statom_flaw &flaw, riddle::predicate &pred, bool is_fact, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept : riddle::atom(pred, is_fact, std::move(args), std::move(sigma)), flaw(flaw) {}
    riddle::bool_expr atom::operator==(riddle::expr rhs) const {}

    stsolver::stsolver(std::string_view name) noexcept : graph(name) {}

    riddle::bool_expr stsolver::new_bool() { return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), utils::lit(net.new_var())); }
    riddle::bool_expr stsolver::new_bool(const bool value)
    {
        auto l = value ? utils::TRUE_lit : utils::FALSE_lit;
        return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::move(l));
    }
    utils::lbool stsolver::bool_value(const riddle::bool_itm &expr) const noexcept { return net.value(static_cast<const bool_item &>(expr).get_lit()); }

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

    utils::inf_rational stsolver::arith_value(const riddle::arith_itm &expr) const noexcept
    {
        if (expr.get_type().get_name() == riddle::int_kw || expr.get_type().get_name() == riddle::real_kw)
            return net.arith_value(static_cast<const arith_item &>(expr).get_lin());
        else
            return utils::inf_rational(net.tp_bounds(static_cast<const arith_item &>(expr).get_lin().vars.begin()->first).first);
    }

    riddle::string_expr stsolver::new_string() { return utils::make_s_ptr<string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ""); }
    riddle::string_expr stsolver::new_string(std::string &&value) { return utils::make_s_ptr<string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), std::move(value)); }
    std::string stsolver::string_value(const riddle::string_itm &expr) const noexcept { return static_cast<const string_item &>(expr).get_string(); }

    riddle::enum_expr stsolver::new_enum(riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values)
    {
        assert(!values.empty());
        return utils::make_s_ptr<enum_item>(static_cast<riddle::enum_type &>(tp), std::move(values), net.new_int(utils::rational::zero, utils::rational(values.size() - 1)));
    }
    std::vector<utils::ref_wrapper<utils::enum_val>> stsolver::enum_value(const riddle::enum_itm &expr) const noexcept { return {static_cast<utils::enum_val &>(*expr.get_values()[net.arith_value(static_cast<const enum_item &>(expr).get_var()).get_rational().numerator()])}; }

    riddle::arith_expr stsolver::new_negation(riddle::arith_expr xpr)
    {
        if (xpr->get_type().get_name() == riddle::int_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), -static_cast<const arith_item &>(*xpr).get_lin());
        else if (xpr->get_type().get_name() == riddle::real_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), -static_cast<const arith_item &>(*xpr).get_lin());
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr stsolver::new_sum(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin sum;
        for (const riddle::arith_expr &xpr : xprs)
            sum += static_cast<const arith_item &>(*xpr).get_lin();
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(sum));
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(sum));
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr stsolver::new_subtraction(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin sub = static_cast<const arith_item &>(*xprs[0]).get_lin();
        for (size_t i = 1; i < xprs.size(); i++)
            sub -= static_cast<const arith_item &>(*xprs[i]).get_lin();
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(sub));
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(sub));
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr stsolver::new_product(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin prod;
        for (const riddle::arith_expr &xpr : xprs)
            if (static_cast<const arith_item &>(*xpr).get_lin().vars.empty())
                prod *= static_cast<const arith_item &>(*xpr).get_lin().known_term;
            else
                throw std::runtime_error("Non-linear arithmetic not supported");
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(prod));
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(prod));
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr stsolver::new_division(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin div = static_cast<const arith_item &>(*xprs[0]).get_lin();
        for (size_t i = 1; i < xprs.size(); i++)
            if (static_cast<const arith_item &>(*xprs[i]).get_lin().vars.empty())
                div /= static_cast<const arith_item &>(*xprs[i]).get_lin().known_term;
            else
                throw std::runtime_error("Non-linear arithmetic not supported");
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(div));
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(div));
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::bool_expr stsolver::new_lt(riddle::arith_expr lhs, riddle::arith_expr rhs)
    {
        assert(!get_current_resolver() || net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) != utils::False);
        auto l = get_current_resolver().has_value() ? static_cast<stresolver &>(*get_current_resolver().value()).get_rho() : utils::TRUE_lit;
        net.new_lt(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin(), utils::lit(l));
        return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::move(l));
    }

    riddle::bool_expr stsolver::new_le(riddle::arith_expr lhs, riddle::arith_expr rhs)
    {
        assert(!get_current_resolver() || net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) != utils::False);
        auto l = get_current_resolver().has_value() ? static_cast<stresolver &>(*get_current_resolver().value()).get_rho() : utils::TRUE_lit;
        net.new_le(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin(), utils::lit(l));
        return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::move(l));
    }

    riddle::bool_expr stsolver::new_gt(riddle::arith_expr lhs, riddle::arith_expr rhs)
    {
        assert(!get_current_resolver() || net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) != utils::False);
        auto l = get_current_resolver().has_value() ? static_cast<stresolver &>(*get_current_resolver().value()).get_rho() : utils::TRUE_lit;
        net.new_gt(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin(), utils::lit(l));
        return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::move(l));
    }

    riddle::bool_expr stsolver::new_ge(riddle::arith_expr lhs, riddle::arith_expr rhs)
    {
        assert(!get_current_resolver() || net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) != utils::False);
        auto l = get_current_resolver().has_value() ? static_cast<stresolver &>(*get_current_resolver().value()).get_rho() : utils::TRUE_lit;
        net.new_ge(static_cast<arith_item &>(*lhs).get_lin(), static_cast<arith_item &>(*rhs).get_lin(), utils::lit(l));
        return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::move(l));
    }

    void stsolver::new_disjunction(std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<utils::ref_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        new_flaw<stdisjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
    }

    void stsolver::assert_clause(std::vector<riddle::bool_expr> &&exprs)
    {
        assert(!exprs.empty());
        std::vector<utils::lit> clause;
        for (const riddle::bool_expr &expr : exprs)
            clause.push_back(static_cast<const bool_item &>(*expr).get_lit());
        if (get_current_resolver().has_value())
            clause.push_back(!static_cast<stresolver &>(*get_current_resolver().value()).get_rho());
        net.new_clause(std::move(clause));
    }

    riddle::atom_expr stsolver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args)
    {
        std::vector<utils::ref_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        auto &af = new_flaw<statom_flaw>(*this, std::move(causes), is_fact, pred, std::move(args));
        return af.get_atom();
    }

    void stsolver::added_causal_link(flaw &f, resolver &r)
    { // if the resolver is active, then the flaw must be active..
        net.new_clause({!static_cast<stresolver &>(r).get_rho(), static_cast<stflaw &>(f).get_phi()});
    }

    bool stsolver::solve()
    {
        net.propagate();
        build(); // we build the causal graph..

        while (true)
        { // we try to solve the problem with the current causal graph..
        }
    }
} // namespace ratio