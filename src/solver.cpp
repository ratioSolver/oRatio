#include "solver.hpp"
#include "graph.hpp"
#include <cassert>

namespace ratio
{
    solver::solver(std::string_view name) noexcept : riddle::core(name)
    {
        assigns.push_back(utils::False); // the false constant..
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
    utils::lbool solver::bool_value(const riddle::bool_term &expr) const noexcept { return value(static_cast<const riddle::bool_item &>(expr).get_lit()); }

    riddle::arith_expr solver::new_int() { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(lin_slv.new_var(), utils::rational::one)); }
    riddle::arith_expr solver::new_int(const INT_TYPE value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::rational(value)); }
    riddle::arith_expr solver::new_int(const INT_TYPE lb, const INT_TYPE ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), lin_slv.new_var(utils::rational(lb), utils::rational(ub))); }
    riddle::arith_expr solver::new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), lin_slv.new_var(utils::rational(lb), utils::rational(ub))); }

    riddle::arith_expr solver::new_real() { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(lin_slv.new_var(), utils::rational::one)); }
    riddle::arith_expr solver::new_real(utils::rational &&value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(value)); }
    riddle::arith_expr solver::new_real(utils::rational &&lb, utils::rational &&ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), lin_slv.new_var(std::move(lb), std::move(ub))); }
    riddle::arith_expr solver::new_uncertain_real(utils::rational &&lb, utils::rational &&ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), lin_slv.new_var(std::move(lb), std::move(ub))); }

    riddle::arith_expr solver::new_time() { return std::make_shared<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), utils::lin(lin_slv.new_var(), utils::rational::one)); }
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

    riddle::arith_expr solver::new_negation(riddle::arith_expr xpr)
    {
        if (xpr->get_type().get_name() == riddle::int_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), -static_cast<const riddle::arith_item &>(*xpr).get_lin());
        else if (xpr->get_type().get_name() == riddle::real_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), -static_cast<const riddle::arith_item &>(*xpr).get_lin());
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr solver::new_sum(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin sum;
        for (const riddle::arith_expr &xpr : xprs)
            sum += static_cast<const riddle::arith_item &>(*xpr).get_lin();
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(sum));
        else if (tp.get_name() == riddle::real_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(sum));
        else
            throw std::runtime_error("Invalid type");
    }
    riddle::arith_expr solver::new_subtraction(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin sub = static_cast<const riddle::arith_item &>(*xprs[0]).get_lin();
        for (size_t i = 1; i < xprs.size(); i++)
            sub -= static_cast<const riddle::arith_item &>(*xprs[i]).get_lin();
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(sub));
        else if (tp.get_name() == riddle::real_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(sub));
        else
            throw std::runtime_error("Invalid type");
    }
    riddle::arith_expr solver::new_product(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin prod;
        for (const riddle::arith_expr &xpr : xprs)
            if (static_cast<const riddle::arith_item &>(*xpr).get_lin().vars.empty())
                prod *= static_cast<const riddle::arith_item &>(*xpr).get_lin().known_term;
            else
                throw std::runtime_error("Non-linear arithmetic not supported");
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(prod));
        else if (tp.get_name() == riddle::real_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(prod));
        else
            throw std::runtime_error("Invalid type");
    }
    riddle::arith_expr solver::new_division(std::vector<riddle::arith_expr> &&xprs)
    {
        assert(xprs.size() > 1);
        utils::lin div = static_cast<const riddle::arith_item &>(*xprs[0]).get_lin();
        for (size_t i = 1; i < xprs.size(); i++)
            if (static_cast<const riddle::arith_item &>(*xprs[i]).get_lin().vars.empty())
                div /= static_cast<const riddle::arith_item &>(*xprs[i]).get_lin().known_term;
            else
                throw std::runtime_error("Non-linear arithmetic not supported");
        auto &tp = type_promotion(xprs);
        if (tp.get_name() == riddle::int_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(div));
        else if (tp.get_name() == riddle::real_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(div));
        else
            throw std::runtime_error("Invalid type");
    }

    void solver::new_clause(std::vector<riddle::bool_expr> &&exprs)
    {
        assert(!exprs.empty());
        if (exprs.size() == 1)
        {
            if (c_res) // if there is a current resolver, add the expression to it..
                c_res.value().get().exprs.push_back(exprs[0]);
            else // otherwise, just execute the expression..
                execute(exprs[0]);
        }
        else
        { // otherwise, create a new clause flaw..
            std::vector<std::reference_wrapper<resolver>> causes;
            if (c_res)
                causes.push_back(c_res.value());

            new_flaw<clause_flaw>(*this, std::move(causes), std::move(exprs));
        }
    }
    void solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<std::reference_wrapper<resolver>> causes;
        if (c_res)
            causes.push_back(c_res.value());

        new_flaw<disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
    }

    void solver::solve()
    {
        if (!lin_slv.check())
            throw std::runtime_error("Unsatisfiable constraints");
    }

    riddle::atom_expr solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args)
    {
        std::vector<std::reference_wrapper<resolver>> causes;
        if (c_res)
            causes.push_back(c_res.value());

        auto &af = new_flaw<atom_flaw>(*this, std::move(causes), is_fact, pred, std::move(args), mk_var());
        return af.get_atom();
    }

    utils::var solver::mk_var() noexcept
    {
        const auto x = assigns.size();
        assigns.push_back(utils::Undefined);
        return x;
    }

    void solver::execute(const riddle::bool_expr &expr)
    {
        if (auto n_xpr = std::dynamic_pointer_cast<riddle::bool_not>(expr))
        {
        }
        else
        {
            if (auto lt_xpr = std::dynamic_pointer_cast<riddle::lt_term>(expr))
            {
                if (lin_slv.new_lt(std::dynamic_pointer_cast<riddle::arith_item>(lt_xpr->get_lhs())->get_lin(), std::dynamic_pointer_cast<riddle::arith_item>(lt_xpr->get_rhs())->get_lin(), true))
                    return;
                else
                    throw std::runtime_error("Unsatisfiable constraint");
            }
            else if (auto le_xpr = std::dynamic_pointer_cast<riddle::le_term>(expr))
            {
                if (lin_slv.new_lt(std::dynamic_pointer_cast<riddle::arith_item>(le_xpr->get_lhs())->get_lin(), std::dynamic_pointer_cast<riddle::arith_item>(le_xpr->get_rhs())->get_lin()))
                    return;
                else
                    throw std::runtime_error("Unsatisfiable constraint");
            }
            else if (auto eq_xpr = std::dynamic_pointer_cast<riddle::eq_term>(expr))
            {
                if (lin_slv.new_eq(std::dynamic_pointer_cast<riddle::arith_item>(eq_xpr->get_lhs())->get_lin(), std::dynamic_pointer_cast<riddle::arith_item>(eq_xpr->get_rhs())->get_lin()))
                    return;
                else
                    throw std::runtime_error("Unsatisfiable constraint");
            }
            else if (auto ge_xpr = std::dynamic_pointer_cast<riddle::ge_term>(expr))
            {
                if (lin_slv.new_gt(std::dynamic_pointer_cast<riddle::arith_item>(ge_xpr->get_lhs())->get_lin(), std::dynamic_pointer_cast<riddle::arith_item>(ge_xpr->get_rhs())->get_lin()))
                    return;
                else
                    throw std::runtime_error("Unsatisfiable constraint");
            }
            else if (auto gt_xpr = std::dynamic_pointer_cast<riddle::gt_term>(expr))
            {
                if (lin_slv.new_gt(std::dynamic_pointer_cast<riddle::arith_item>(gt_xpr->get_lhs())->get_lin(), std::dynamic_pointer_cast<riddle::arith_item>(gt_xpr->get_rhs())->get_lin(), true))
                    return;
                else
                    throw std::runtime_error("Unsatisfiable constraint");
            }
            else
                throw std::runtime_error("Unsupported boolean expression");
        }
    }
} // namespace ratio
