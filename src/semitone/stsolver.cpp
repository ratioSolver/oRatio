#include "stsolver.hpp"
#include "stflaws.hpp"
#include "sttypes.hpp"
#include "conjunction.hpp"
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

    riddle::bool_expr stsolver::new_and(std::vector<riddle::bool_expr> &&exprs)
    {
        assert(!exprs.empty());
        if (get_current_resolver())
        { // activating the resolver will activate the conjunction..
            for (const riddle::bool_expr &expr : exprs)
                net.add_clause({!static_cast<const stresolver &>(*get_current_resolver().value()).get_rho(), static_cast<const bool_item &>(*expr).get_expr()});
            return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), static_cast<const stresolver &>(*get_current_resolver().value()).get_rho());
        }
        else
        { // the conjunction must be activated independently..
            for (const riddle::bool_expr &expr : exprs)
                net.add_clause({static_cast<const bool_item &>(*expr).get_expr()});
            return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), utils::TRUE_lit);
        }
    }

    riddle::bool_expr stsolver::new_or(std::vector<riddle::bool_expr> &&exprs)
    {
        assert(!exprs.empty());
        std::vector<utils::ref_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        std::vector<utils::lit> lits;
        for (const riddle::bool_expr &expr : exprs)
            lits.push_back(static_cast<const bool_item &>(*expr).get_expr());
        auto &f = new_flaw<stclause_flaw>(*this, std::move(causes), std::move(lits), false);
        return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), f.get_phi());
    }

    riddle::bool_expr stsolver::new_xor(std::vector<riddle::bool_expr> &&exprs)
    {
        assert(!exprs.empty());
        std::vector<utils::ref_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        std::vector<utils::lit> lits;
        for (const riddle::bool_expr &expr : exprs)
            lits.push_back(static_cast<const bool_item &>(*expr).get_expr());
        auto &f = new_flaw<stclause_flaw>(*this, std::move(causes), std::move(lits), true);
        return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), f.get_phi());
    }

    riddle::bool_expr stsolver::new_not(riddle::bool_expr expr) { return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), !static_cast<const bool_item &>(*expr).get_expr()); }

    riddle::arith_expr stsolver::new_negation(riddle::arith_expr xpr)
    {
        if (xpr->get_type().get_name() == riddle::int_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), -static_cast<const arith_item &>(*xpr).get_expr());
        else if (xpr->get_type().get_name() == riddle::real_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), -static_cast<const arith_item &>(*xpr).get_expr());
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr stsolver::new_sum(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin sum;
        for (const riddle::arith_expr &xpr : xprs)
            sum += static_cast<const arith_item &>(*xpr).get_expr();
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), sum);
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), sum);
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr stsolver::new_subtraction(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin sub = static_cast<const arith_item &>(*xprs[0]).get_expr();
        for (size_t i = 1; i < xprs.size(); i++)
            sub -= static_cast<const arith_item &>(*xprs[i]).get_expr();
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), sub);
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), sub);
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr stsolver::new_product(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin prod;
        for (const riddle::arith_expr &xpr : xprs)
            if (static_cast<const arith_item &>(*xpr).get_expr().vars.empty())
                prod *= static_cast<const arith_item &>(*xpr).get_expr().known_term;
            else
                throw std::runtime_error("Non-linear arithmetic not supported");
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), prod);
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), prod);
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr stsolver::new_division(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin xpr = static_cast<const arith_item &>(*xprs[0]).get_expr();
        for (size_t i = 1; i < xprs.size(); i++)
            if (static_cast<const arith_item &>(*xprs[i]).get_expr().vars.empty())
                xpr /= static_cast<const arith_item &>(*xprs[i]).get_expr().known_term;
            else
                throw std::runtime_error("Non-linear arithmetic not supported");
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), xpr);
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), xpr);
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::bool_expr stsolver::new_lt(riddle::arith_expr lhs, riddle::arith_expr rhs)
    {
        assert(!get_current_resolver() || net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) == utils::False);
        if (get_current_resolver() && net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) == utils::Undefined)
        { // activating the resolver will activate the comparison..
            net.new_lt(utils::lit(static_cast<stresolver &>(*get_current_resolver().value()).get_rho()), utils::lin(static_cast<arith_item &>(*lhs).get_expr()), utils::lin(static_cast<arith_item &>(*rhs).get_expr()));
            return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), static_cast<const stresolver &>(*get_current_resolver().value()).get_rho());
        }
        else
        { // the comparison must be activated independently..
            net.add_lt(utils::lin(static_cast<arith_item &>(*lhs).get_expr()), utils::lin(static_cast<arith_item &>(*rhs).get_expr()));
            return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), utils::TRUE_lit);
        }
    }

    riddle::bool_expr stsolver::new_le(riddle::arith_expr lhs, riddle::arith_expr rhs)
    {
        assert(!get_current_resolver() || net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) == utils::False);
        if (get_current_resolver() && net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) == utils::Undefined)
        { // activating the resolver will activate the comparison..
            net.new_le(utils::lit(static_cast<stresolver &>(*get_current_resolver().value()).get_rho()), utils::lin(static_cast<arith_item &>(*lhs).get_expr()), utils::lin(static_cast<arith_item &>(*rhs).get_expr()));
            return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), static_cast<const stresolver &>(*get_current_resolver().value()).get_rho());
        }
        else
        { // the comparison must be activated independently..
            net.add_le(utils::lin(static_cast<arith_item &>(*lhs).get_expr()), utils::lin(static_cast<arith_item &>(*rhs).get_expr()));
            return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), utils::TRUE_lit);
        }
    }

    riddle::bool_expr stsolver::new_gt(riddle::arith_expr lhs, riddle::arith_expr rhs)
    {
        assert(!get_current_resolver() || net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) == utils::False);
        if (get_current_resolver() && net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) == utils::Undefined)
        { // activating the resolver will activate the comparison..
            net.new_gt(utils::lit(static_cast<stresolver &>(*get_current_resolver().value()).get_rho()), utils::lin(static_cast<arith_item &>(*lhs).get_expr()), utils::lin(static_cast<arith_item &>(*rhs).get_expr()));
            return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), static_cast<const stresolver &>(*get_current_resolver().value()).get_rho());
        }
        else
        { // the comparison must be activated independently..
            net.add_gt(utils::lin(static_cast<arith_item &>(*lhs).get_expr()), utils::lin(static_cast<arith_item &>(*rhs).get_expr()));
            return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), utils::TRUE_lit);
        }
    }

    riddle::bool_expr stsolver::new_ge(riddle::arith_expr lhs, riddle::arith_expr rhs)
    {
        assert(!get_current_resolver() || net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) == utils::False);
        if (get_current_resolver() && net.value(static_cast<const stresolver &>(*get_current_resolver().value()).get_rho()) == utils::Undefined)
        { // activating the resolver will activate the comparison..
            net.new_ge(utils::lit(static_cast<stresolver &>(*get_current_resolver().value()).get_rho()), utils::lin(static_cast<arith_item &>(*lhs).get_expr()), utils::lin(static_cast<arith_item &>(*rhs).get_expr()));
            return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), static_cast<const stresolver &>(*get_current_resolver().value()).get_rho());
        }
        else
        { // the comparison must be activated independently..
            net.add_ge(utils::lin(static_cast<arith_item &>(*lhs).get_expr()), utils::lin(static_cast<arith_item &>(*rhs).get_expr()));
            return utils::make_s_ptr<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), utils::TRUE_lit);
        }
    }

    void stsolver::new_disjunction(std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<utils::ref_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        new_flaw<stdisjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
    }

    void stsolver::assert_fact(riddle::bool_expr fact)
    {
        if (get_current_resolver().has_value())
            net.add_clause({!static_cast<stresolver &>(*get_current_resolver().value()).get_rho(), static_cast<const bool_item &>(*fact).get_expr()});
        else
            net.add_clause({static_cast<const bool_item &>(*fact).get_expr()});
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
        net.add_clause({!static_cast<stresolver &>(r).get_rho(), static_cast<stflaw &>(f).get_phi()});
    }

    bool stsolver::solve()
    {
        if (!net.propagate())
            return false; // no solution..

        build(); // we build the causal graph..

        while (true)
        { // we try to solve the problem with the current causal graph..
        }
    }
} // namespace ratio