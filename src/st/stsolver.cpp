#include "stsolver.hpp"
#include "init.hpp"
#include "stflaws.hpp"
#include "sttypes.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <algorithm>
#include <stack>
#include <cassert>

namespace ratio
{
    enum_item::enum_item(riddle::component_type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values, std::vector<utils::lit> &&lits) noexcept : riddle::enum_item(tp, std::move(values), std::move(lits)) {}

    riddle::expr enum_item::get(std::string_view name)
    {
        assert(get_values().size() > 1); // should not be a singleton..

        if (auto it = items.find(name.data()); it != items.end())
            return it->second;

        // different referenced values can represent the same item, so we group them by the item they represent..
        std::unordered_map<riddle::term *, std::pair<riddle::expr, std::vector<utils::lit>>> itm_vars;
        for (const auto &v : get_values())
        {
            auto xpr = dynamic_cast<riddle::env *>(&v.get())->get(name);
            if (auto t = itm_vars.find(&*xpr); t != itm_vars.end())
                t->second.second.emplace_back(get_lit(v.get()));
            else
            {
                auto p = std::make_pair(xpr, std::vector<utils::lit>());
                p.second.emplace_back(get_lit(v.get()));
                itm_vars.emplace(&*xpr, std::move(p));
            }
        }

        assert(!itm_vars.empty());
        assert(std::none_of(itm_vars.begin(), itm_vars.end(), [](const auto &ivs)
                            { return ivs.second.second.empty(); }));

        if (itm_vars.size() == 1)
        { // we are lucky!
            items.emplace(name, itm_vars.begin()->second.first);
            return itm_vars.begin()->second.first;
        }
        // we have to create a new variable :(

        auto &tp = static_cast<riddle::component_type &>(get_type()).get_field(name).get_type(); // the target type..

        if (is_bool(tp))
        { // we create a new boolean item..
            auto b = std::dynamic_pointer_cast<riddle::bool_item>(get_core().new_bool());
            // we force the variable to assume the same value of the referenced bools according to the value of the enum..
            for (const auto &[t, vs] : itm_vars)
                for (const auto &v : vs.second)
                {
                    std::vector<utils::lit> c_vars_0;
                    c_vars_0.emplace_back(!v);
                    c_vars_0.emplace_back(!b->get_lit());
                    c_vars_0.emplace_back(static_cast<riddle::bool_item &>(*t).get_lit());
                    static_cast<solver &>(get_core()).add_clause(std::move(c_vars_0));
                    std::vector<utils::lit> c_vars_1;
                    c_vars_1.emplace_back(!v);
                    c_vars_1.emplace_back(b->get_lit());
                    c_vars_1.emplace_back(!static_cast<riddle::bool_item &>(*t).get_lit());
                    static_cast<solver &>(get_core()).add_clause(std::move(c_vars_1));
                }
            items.emplace(name, b);
            return b;
        }
        else if (is_int(tp) || is_real(tp))
        {
            auto min = utils::inf_rational(utils::rational::positive_infinite);
            auto max = utils::inf_rational(utils::rational::negative_infinite);
            for (const auto &[t, vs] : itm_vars)
            {
                const auto &a_itm = static_cast<riddle::arith_item &>(*t);
                const auto c_min = static_cast<solver &>(get_core()).arith_lb(a_itm.get_lin());
                if (min < c_min)
                    min = c_min;
                const auto c_max = static_cast<solver &>(get_core()).arith_ub(a_itm.get_lin());
                if (max > c_max)
                    max = c_max;
            }
            if (min == max)
            { // we are lucky! we have a constant..
                if (is_int(tp))
                {
                    assert(min.get_infinitesimal() == 0);
                    assert(min.get_rational().denominator() == 1);
                    auto i = get_core().new_int(min.get_rational().numerator());
                    items.emplace(name, i);
                    return i;
                }
                else
                {
                    assert(is_real(tp));
                    assert(min.get_infinitesimal() == 0);
                    auto i = get_core().new_real(min.get_rational());
                    items.emplace(name, i);
                    return i;
                }
            }
            else
            { // we need to create a new variable..
                auto ai = is_int(tp) ? std::dynamic_pointer_cast<riddle::arith_item>(get_core().new_int()) : std::dynamic_pointer_cast<riddle::arith_item>(get_core().new_real());
                // we force the variable to assume the same value of the referenced ariths according to the value of the enum..
                for (const auto &[t, vs] : itm_vars)
                    for (const auto &v : vs.second)
                        static_cast<solver &>(get_core()).add_eq(ai->get_lin(), static_cast<riddle::arith_item &>(*t).get_lin(), v);
                items.emplace(name, ai);
                return ai;
            }
        }
        else
        {
            std::vector<std::reference_wrapper<utils::enum_val>> values;
            std::vector<utils::lit> lits;
            for (const auto &[itm, vars] : itm_vars)
            {
                values.push_back(*itm);
                if (vars.second.size() == 1)
                    lits.emplace_back(*vars.second.begin());
                else
                {
                    const auto v = utils::lit(static_cast<solver &>(get_core()).mk_var());
                    std::vector<utils::lit> vs = vars.second;
                    vs.emplace_back(!v);
                    static_cast<solver &>(get_core()).add_clause(std::move(vs));
                    lits.emplace_back(v);
                }
            }

            auto ei = std::make_shared<enum_item>(static_cast<riddle::component_type &>(tp), std::move(values), std::move(lits));
            items.emplace(name, ei);
            return ei;
        }
    }

    riddle::atom_state atom::get_state() const noexcept
    {
        switch (static_cast<solver &>(flaw.get_graph()).value(get_sigma()))
        {
        case utils::True:
            return riddle::active;
        case utils::False:
            return riddle::unified;
        default:
            return riddle::inactive;
        }
    }

    solver::solver(std::string_view name) noexcept : graph(name)
    {
        read(INIT_STRING);
        add_type(std::make_unique<ststate_variable>(*this));
        add_type(std::make_unique<streusable_resource>(*this));
        add_type(std::make_unique<stconsumable_resource>(*this));
    }

    riddle::bool_expr solver::new_bool() { return std::make_shared<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), utils::lit(mk_var())); }
    riddle::bool_expr solver::new_bool(const bool value)
    {
        auto l = value ? utils::TRUE_lit : utils::FALSE_lit;
        return std::make_shared<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::move(l));
    }
    utils::lbool solver::bool_value(const riddle::bool_term &expr) const noexcept { return value(static_cast<const riddle::bool_item &>(expr).get_lit()); }

    riddle::arith_expr solver::new_int() { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(mk_int(), utils::rational::one)); }
    riddle::arith_expr solver::new_int(const INT_TYPE value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(utils::rational(value))); }
    riddle::arith_expr solver::new_int(const INT_TYPE lb, const INT_TYPE ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(mk_int(utils::rational(lb), utils::rational(ub)), utils::rational::one)); }
    riddle::arith_expr solver::new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(mk_int(utils::rational(lb), utils::rational(ub)), utils::rational::one)); }

    riddle::arith_expr solver::new_real() { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(mk_real(), utils::rational::one)); }
    riddle::arith_expr solver::new_real(utils::rational &&value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(std::move(value))); }
    riddle::arith_expr solver::new_real(utils::rational &&lb, utils::rational &&ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(mk_real(std::move(lb), std::move(ub)), utils::rational::one)); }
    riddle::arith_expr solver::new_uncertain_real(utils::rational &&lb, utils::rational &&ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(mk_real(std::move(lb), std::move(ub)), utils::rational::one)); }

    riddle::arith_expr solver::new_time() { return std::make_shared<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), utils::lin(mk_tp(), utils::rational::one)); }
    riddle::arith_expr solver::new_time(utils::rational &&value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), utils::lin(std::move(value))); }

    utils::inf_rational solver::arith_value(const riddle::arith_term &expr) const noexcept
    {
        if (expr.get_type().get_name() == riddle::int_kw || expr.get_type().get_name() == riddle::real_kw)
            return arith_val(static_cast<const riddle::arith_item &>(expr).get_lin());
        else
            return utils::inf_rational(tp_bounds(static_cast<const riddle::arith_item &>(expr).get_lin().vars.begin()->first).first);
    }

    riddle::string_expr solver::new_string() { return std::make_shared<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ""); }
    riddle::string_expr solver::new_string(std::string &&value) { return std::make_shared<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), std::move(value)); }
    std::string solver::string_value(const riddle::string_term &expr) const noexcept { return static_cast<const riddle::string_item &>(expr).get_string(); }

    riddle::enum_expr solver::new_enum(riddle::component_type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values)
    {
        std::vector<std::reference_wrapper<resolver>> causes;
        if (get_current_resolver())
            causes.push_back(get_current_resolver().value());
        auto &ef = new_flaw<enum_flaw>(*this, std::move(causes), tp, std::move(values));
        return ef.get_var();
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

    void solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<std::reference_wrapper<resolver>> causes;
        if (get_current_resolver())
            causes.push_back(get_current_resolver().value());
        new_flaw<disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
    }

    void solver::new_clause(std::vector<riddle::bool_expr> &&exprs)
    {
        assert(!exprs.empty());
        std::vector<utils::lit> clause;
        for (const riddle::bool_expr &expr : exprs)
            if (auto b_xpr = std::dynamic_pointer_cast<riddle::bool_item>(expr))
                clause.push_back(b_xpr->get_lit());
            else if (auto n_xpr = std::dynamic_pointer_cast<riddle::bool_not>(expr))
            {
                if (auto b_xpr = std::dynamic_pointer_cast<riddle::bool_item>(n_xpr->get_arg()))
                    clause.push_back(!b_xpr->get_lit());
                else
                {
                    utils::lit p;
                    if (exprs.size() > 1)
                    { // we create a new variable for the constraint..
                        p = utils::lit(mk_var());
                        clause.push_back(p);
                    }
                    else if (get_current_resolver()) // we add the constraint to the current resolver..
                        p = static_cast<stresolver &>(get_current_resolver().value().get()).get_rho();
                    else // we enforce the constraint directly..
                        p = utils::TRUE_lit;

                    if (auto lt_xpr = std::dynamic_pointer_cast<riddle::lt_term>(n_xpr->get_arg()))
                        add_ge(static_cast<riddle::arith_item &>(*lt_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*lt_xpr->get_rhs()).get_lin(), p);
                    else if (auto le_xpr = std::dynamic_pointer_cast<riddle::le_term>(n_xpr->get_arg()))
                        add_gt(static_cast<riddle::arith_item &>(*le_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*le_xpr->get_rhs()).get_lin(), p);
                    else if (auto eq_xpr = std::dynamic_pointer_cast<riddle::eq_term>(n_xpr->get_arg()))
                        make_neq(*eq_xpr->get_lhs(), *eq_xpr->get_rhs(), p);
                    else if (auto ge_xpr = std::dynamic_pointer_cast<riddle::ge_term>(n_xpr->get_arg()))
                        add_lt(static_cast<riddle::arith_item &>(*ge_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*ge_xpr->get_rhs()).get_lin(), p);
                    else if (auto gt_xpr = std::dynamic_pointer_cast<riddle::gt_term>(n_xpr->get_arg()))
                        add_le(static_cast<riddle::arith_item &>(*gt_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*gt_xpr->get_rhs()).get_lin(), p);
                    else
                        throw std::runtime_error("Invalid type");
                }
            }
            else
            {
                utils::lit p;
                if (exprs.size() > 1)
                { // we create a new variable for the constraint..
                    p = utils::lit(mk_var());
                    clause.push_back(p);
                }
                else if (get_current_resolver()) // we add the constraint to the current resolver..
                    p = static_cast<stresolver &>(get_current_resolver().value().get()).get_rho();
                else // we enforce the constraint directly..
                    p = utils::TRUE_lit;

                if (auto lt_xpr = std::dynamic_pointer_cast<riddle::lt_term>(expr))
                    add_lt(static_cast<riddle::arith_item &>(*lt_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*lt_xpr->get_rhs()).get_lin(), p);
                else if (auto le_xpr = std::dynamic_pointer_cast<riddle::le_term>(expr))
                    add_le(static_cast<riddle::arith_item &>(*le_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*le_xpr->get_rhs()).get_lin(), p);
                else if (auto eq_xpr = std::dynamic_pointer_cast<riddle::eq_term>(expr))
                    make_eq(*eq_xpr->get_lhs(), *eq_xpr->get_rhs(), p);
                else if (auto ge_xpr = std::dynamic_pointer_cast<riddle::ge_term>(expr))
                    add_ge(static_cast<riddle::arith_item &>(*ge_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*ge_xpr->get_rhs()).get_lin(), p);
                else if (auto gt_xpr = std::dynamic_pointer_cast<riddle::gt_term>(expr))
                    add_gt(static_cast<riddle::arith_item &>(*gt_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*gt_xpr->get_rhs()).get_lin(), p);
                else
                    throw std::runtime_error("Invalid type");
            }

        if (clause.size() == 1)
        { // we can propagate..
            if (get_current_resolver())
                clause.push_back(!static_cast<stresolver &>(get_current_resolver().value().get()).get_rho());
            add_clause(std::move(clause));
        }
        else if (clause.size() > 1)
        { // we have a new flaw..
            std::vector<std::reference_wrapper<resolver>> causes;
            if (get_current_resolver())
                causes.emplace_back(get_current_resolver().value());
            new_flaw<clause_flaw>(*this, std::move(causes), std::move(clause), false);
        }
    }

    riddle::atom_expr solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args)
    {
        std::vector<std::reference_wrapper<resolver>> causes;
        if (get_current_resolver())
            causes.push_back(get_current_resolver().value());
        auto &af = new_flaw<atom_flaw>(*this, std::move(causes), is_fact, pred, std::move(args));
        return af.get_atom();
    }

    void solver::added_causal_link(flaw &f, resolver &r)
    { // if the resolver is active, then the flaw must be active..
        add_clause({!static_cast<stresolver &>(r).get_rho(), static_cast<stflaw &>(f).get_phi()});

        // the activation of the resolver enforces the activation of a distance constraint to avoid the creation of cycles..
        add_distance(static_cast<stflaw &>(f).get_pos(), static_cast<stflaw &>(r.get_flaw()).get_pos(), -utils::rational::one, static_cast<stresolver &>(r).get_rho());
    }

    bool solver::match(riddle::term &lhs, riddle::term &rhs) const
    {
        if (&lhs == &rhs) // the terms are the same, so they match..
            return true;
        else if (&lhs.get_type() != &rhs.get_type()) // the types are different, so the terms cannot match..
            return false;
        else if (auto lhs_xpr = dynamic_cast<riddle::arith_item *>(&lhs)) // we are dealing with an arithmetic comparison..
            return arith_lb(lhs_xpr->get_lin()) <= arith_ub(static_cast<riddle::arith_item &>(rhs).get_lin()) && arith_ub(lhs_xpr->get_lin()) >= arith_lb(static_cast<riddle::arith_item &>(rhs).get_lin());
        else if (auto lhs_xpr = dynamic_cast<riddle::bool_item *>(&lhs)) // we are dealing with a boolean comparison..
            return value(lhs_xpr->get_lit()) == value(static_cast<riddle::bool_item &>(rhs).get_lit()) || value(lhs_xpr->get_lit()) == utils::Undefined || value(static_cast<riddle::bool_item &>(rhs).get_lit()) == utils::Undefined;
        else if (auto lhs_xpr = dynamic_cast<riddle::string_item *>(&lhs)) // we are dealing with a string comparison..
            return lhs_xpr->get_string() == static_cast<riddle::string_item &>(rhs).get_string();
        else if (auto lhs_xpr = dynamic_cast<riddle::enum_item *>(&lhs))
        { // we are dealing with an enumeration comparison..
            if (auto rhs_xpr = dynamic_cast<riddle::enum_item *>(&rhs))
            {
                // we compute the intersection of the two domains
                std::unordered_set<utils::enum_val *> intersection;
                for (const auto &v : lhs_xpr->get_values())
                    for (const auto &w : rhs_xpr->get_values())
                        if (&v.get() == &w.get())
                        {
                            intersection.insert(&v.get());
                            break;
                        }
                return std::any_of(intersection.begin(), intersection.end(), [&](const utils::enum_val *v)
                                   { return value(lhs_xpr->get_lit(*v)) == value(rhs_xpr->get_lit(*v)) || value(lhs_xpr->get_lit(*v)) == utils::Undefined || value(rhs_xpr->get_lit(*v)) == utils::Undefined; });
            }
            else
            { // we are dealing with an enumeration and a constant..
                for (const auto &v : lhs_xpr->get_values())
                    if (value(lhs_xpr->get_lit(v.get())) != utils::False)
                        if (match(static_cast<riddle::term &>(v.get()), rhs)) // if any of the values match, then we are done..
                            return true;
                return false;
            }
        }
        else if (auto rhs_xpr = dynamic_cast<riddle::enum_item *>(&rhs)) // we are comparing a constant with an enum item..
            return match(*rhs_xpr, lhs);
        else if (auto lhs_xpr = dynamic_cast<riddle::atom_term *>(&lhs))
        { // we are dealing with atoms..
            auto rhs_xpr = static_cast<riddle::atom_term *>(&rhs);
            if (&lhs_xpr->get_type().get_scope() != &rhs_xpr->get_type().get_scope().get_core() && !match(*lhs_xpr->get(riddle::tau_kw), *rhs_xpr->get(riddle::tau_kw)))
                return false; // the atoms are not in the same scope, so they cannot match..
            // we check if the atoms' fields match..
            std::queue<riddle::predicate *> q;
            q.push(static_cast<riddle::predicate *>(&lhs_xpr->get_type()));
            while (!q.empty())
            {
                for (const auto &[f_name, f] : q.front()->get_fields())
                    if (!match(*lhs_xpr->get(f_name), *rhs_xpr->get(f_name)))
                        return false;
                for (const auto &pp : q.front()->get_parents())
                    q.push(&pp.get());
                q.pop();
            }
            return true;
        }
        else // we are dealing with components (and we have already checked their are not the same)..
            return false;
    }

    void solver::make_eq(riddle::term &lhs, riddle::term &rhs, const utils::lit &p)
    {
        if (&lhs == &rhs) // the terms are the same, so they are equal..
            return;
        else if (&lhs.get_type() != &rhs.get_type()) // the types are different, so the constraint is always false..
            add_clause({!p});
        else if (auto lhs_xpr = dynamic_cast<riddle::arith_item *>(&lhs)) // we are dealing with an arithmetic constraint..
            add_eq(lhs_xpr->get_lin(), static_cast<riddle::arith_item *>(&rhs)->get_lin(), p);
        else if (auto lhs_xpr = dynamic_cast<riddle::bool_item *>(&lhs))
        { // we are dealing with a boolean constraint..
            auto rhs_xpr = static_cast<riddle::bool_item *>(&rhs);
            add_clause({!p, lhs_xpr->get_lit(), !rhs_xpr->get_lit()});
            add_clause({!p, !lhs_xpr->get_lit(), rhs_xpr->get_lit()});
        }
        else if (auto lhs_xpr = dynamic_cast<riddle::string_item *>(&lhs))
        { // we are dealing with a string constraint..
            if (lhs_xpr->get_string() != static_cast<riddle::string_item *>(&rhs)->get_string())
                add_clause({!p}); // the strings are different, so the constraint is always false..
        }
        else if (auto lhs_xpr = dynamic_cast<riddle::enum_item *>(&lhs))
        { // we are dealing with an enumeration constraint..
            if (auto rhs_xpr = dynamic_cast<riddle::enum_item *>(&rhs))
            {
                // we compute the intersection of the two domains
                std::unordered_set<utils::enum_val *> intersection;
                for (const auto &v : lhs_xpr->get_values())
                    for (const auto &w : rhs_xpr->get_values())
                        if (&v.get() == &w.get())
                        {
                            intersection.insert(&v.get());
                            break;
                        }
                if (intersection.empty())
                    add_clause({!p}); // the domains are disjoint, so the constraint is always false..

                // the values outside the intersection are pruned if the equality control variable becomes true..
                for (const auto &v : lhs_xpr->get_values())
                    if (!intersection.count(&v.get()))
                        add_clause({!lhs_xpr->get_lit(v.get()), !p});
                for (const auto &v : rhs_xpr->get_values())
                    if (!intersection.count(&v.get()))
                        add_clause({!rhs_xpr->get_lit(v.get()), !p});

                for (const auto &v : intersection)
                {
                    add_clause({!p, lhs_xpr->get_lit(*v), !rhs_xpr->get_lit(*v)});
                    add_clause({!p, !lhs_xpr->get_lit(*v), rhs_xpr->get_lit(*v)});
                }
            }
            else
            {
                add_clause({!p, lhs_xpr->get_lit(*static_cast<utils::enum_val *>(&rhs))});
                for (const auto &v : lhs_xpr->get_values())
                    if (&v.get() != static_cast<utils::enum_val *>(&rhs))
                        add_clause({!p, !lhs_xpr->get_lit(v.get())});
            }
        }
        else if (auto rhs_xpr = dynamic_cast<riddle::enum_item *>(&rhs)) // we are dealing with an enumeration constraint..
            make_eq(*rhs_xpr, lhs, p);
        else if (auto lhs_xpr = dynamic_cast<riddle::atom_term *>(&lhs))
        { // we are dealing with atoms..
            auto rhs_xpr = static_cast<riddle::atom_term *>(&rhs);
            std::queue<riddle::predicate *> q;
            q.push(static_cast<riddle::predicate *>(&lhs_xpr->get_type()));
            while (!q.empty())
            {
                for (const auto &[f_name, f] : q.front()->get_fields())
                    if (value(p) == utils::False) // if the equality control variable is false, we do not need to add any further constraints..
                        return;
                    else
                        make_eq(*lhs_xpr->get(f_name), *rhs_xpr->get(f_name), p);
                for (const auto &pp : q.front()->get_parents())
                    q.push(&pp.get());
                q.pop();
            }
        }
        else // we are dealing with components (and we have already checked their are not the same)..
            add_clause({!p});
    }

    void solver::make_neq(riddle::term &lhs, riddle::term &rhs, const utils::lit &p)
    {
        if (&lhs.get_type() != &rhs.get_type()) // the types are different, so the constraint is always true..
            return;
        else if (auto lhs_ai_xpr = dynamic_cast<riddle::arith_item *>(&lhs))
        { // we are dealing with an arithmetic constraint..
            auto rhs_ai_xpr = static_cast<riddle::arith_item *>(&rhs);
            auto lt = utils::lit(mk_var());
            add_lt(lhs_ai_xpr->get_lin(), rhs_ai_xpr->get_lin(), lt);
            auto gt = utils::lit(mk_var());
            add_lt(rhs_ai_xpr->get_lin(), lhs_ai_xpr->get_lin(), gt);
            add_clause({!p, lt, gt});
        }
        else if (auto lhs_bi_xpr = dynamic_cast<riddle::bool_item *>(&lhs)) // we are dealing with a boolean constraint..
        {
            auto rhs_bi_xpr = static_cast<riddle::bool_item *>(&rhs);
            add_clause({!p, lhs_bi_xpr->get_lit(), rhs_bi_xpr->get_lit()});
            add_clause({!p, !lhs_bi_xpr->get_lit(), !rhs_bi_xpr->get_lit()});
        }
        else if (auto lhs_si_xpr = dynamic_cast<riddle::string_item *>(&lhs)) // we are dealing with a string constraint..
        {
            if (lhs_si_xpr->get_string() == static_cast<riddle::string_item *>(&rhs)->get_string()) // the strings are equal, so the constraint is always false..
                add_clause({!p});
        }
        else if (auto lhs_ei_xpr = dynamic_cast<riddle::enum_item *>(&lhs)) // we are dealing with an enumeration constraint..
        {
            if (auto rhs_ei_xpr = dynamic_cast<riddle::enum_item *>(&rhs))
            {
                for (const auto &v : lhs_ei_xpr->get_values())
                    for (const auto &w : rhs_ei_xpr->get_values())
                        if (&v.get() == &w.get())
                        { // choosing a value from one domain excludes the corresponding value from the other domain..
                            add_clause({!p, !lhs_ei_xpr->get_lit(v.get()), !rhs_ei_xpr->get_lit(v.get())});
                            break;
                        }
            }
            else if (lhs_ei_xpr->has_lit(*static_cast<utils::enum_val *>(&rhs)))
                add_clause({!p, !lhs_ei_xpr->get_lit(*static_cast<utils::enum_val *>(&rhs))}); // the value is excluded from the domain..
        }
        else if (auto tmp_rhs_ei_xpr = dynamic_cast<riddle::enum_item *>(&rhs)) // we are dealing with an enumeration constraint..
            make_neq(*tmp_rhs_ei_xpr, lhs, p);
        else if (auto lhs_at_xpr = dynamic_cast<riddle::atom_term *>(&lhs))
        { // we are dealing with atoms..
            std::vector<utils::lit> clause;
            clause.push_back(!p);
            auto rhs_at_xpr = static_cast<riddle::atom_term *>(&rhs);
            std::queue<riddle::predicate *> q;
            q.push(static_cast<riddle::predicate *>(&lhs_at_xpr->get_type()));
            while (!q.empty())
            {
                for (const auto &[f_name, f] : q.front()->get_fields())
                {
                    auto neq = utils::lit(mk_var());
                    make_neq(*lhs_at_xpr->get(f_name), *rhs_at_xpr->get(f_name), neq);
                    clause.push_back(neq);
                }
                for (const auto &pp : q.front()->get_parents())
                    q.push(&pp.get());
                q.pop();
            }
            add_clause(std::move(clause));
        }
        else if (auto lhs_c_xpr = dynamic_cast<riddle::component *>(&lhs))
        { // we are dealing with components..
            std::vector<utils::lit> clause;
            clause.push_back(!p);
            auto rhs_c_xpr = static_cast<riddle::component *>(&rhs);
            std::queue<riddle::component_type *> q;
            q.push(static_cast<riddle::component_type *>(&lhs_c_xpr->get_type()));
            while (!q.empty())
            {
                for (const auto &[f_name, f] : q.front()->get_fields())
                {
                    auto neq = utils::lit(mk_var());
                    make_neq(*lhs_c_xpr->get(f_name), *rhs_c_xpr->get(f_name), neq);
                    clause.push_back(neq);
                }
                for (const auto &pp : q.front()->get_parents())
                    q.push(&pp.get());
                q.pop();
            }
            add_clause(std::move(clause));
        }
        else
            throw std::runtime_error("Invalid type");
    }

    void solver::solve()
    {
        propagate(); // we perform an initial propagation..

        check_graph();

#ifdef CHECK_INCONSISTENCIES
        // we solve all the current inconsistencies..
        solve_inconsistencies();
        check_graph();

        while (!get_active_flaws().empty())
        { // we try to solve the problem with the current causal graph..
            if (std::any_of(get_root_flaws().begin(), get_root_flaws().end(), [](const auto &f)
                            { return is_infinite(f->get_estimated_cost()); }))
            { // we don't know how to solve this flaw :(
                next();
                STATE_CHANGED();
                check_graph();
                continue;
            }

            // we get the most expensive flaw..
            auto f = *std::max_element(get_active_flaws().begin(), get_active_flaws().end(), [](const auto &a, const auto &b)
                                       { return a->get_estimated_cost() < b->get_estimated_cost(); });
            set_current_flaw(*f);
            assert(!is_infinite(f->get_estimated_cost()));
            assert(std::all_of(f->get_resolvers().begin(), f->get_resolvers().end(), [f](const auto &r)
                               { return f == &r->get_flaw(); }));
            assert(std::none_of(f->get_resolvers().begin(), f->get_resolvers().end(), [this](const auto &r)
                                { return value(static_cast<stresolver &>(r.get()).get_rho()) == utils::True; }));

            // we get the least expensive resolver..
            auto r = *std::min_element(f->get_resolvers().begin(), f->get_resolvers().end(), [](const auto &a, const auto &b)
                                       { return a->get_estimated_cost() < b->get_estimated_cost(); });
            assert(!is_infinite(r->get_estimated_cost()));
            set_current_resolver(r.get());

            // we apply the resolver..
            assume(static_cast<stresolver &>(r.get()).get_rho());
            STATE_CHANGED();

            set_current_resolver(std::nullopt);
            set_current_flaw(std::nullopt);

            check_graph();

            // we solve all the current inconsistencies..
            solve_inconsistencies();
            check_graph();
        }
#else
        do
        {
            while (!get_active_flaws().empty())
            { // we try to solve the problem with the current causal graph..
                if (std::any_of(get_root_flaws().begin(), get_root_flaws().end(), [](const auto &f)
                                { return is_infinite(f->get_estimated_cost()); }))
                { // we don't know how to solve this flaw :(
                    next();
                    STATE_CHANGED();
                    check_graph();
                    continue;
                }

                // we get the most expensive flaw..
                auto f = *std::max_element(get_active_flaws().begin(), get_active_flaws().end(), [](const auto &a, const auto &b)
                                           { return a->get_estimated_cost() < b->get_estimated_cost(); });
                set_current_flaw(*f);
                assert(!is_infinite(f->get_estimated_cost()));
                assert(std::all_of(f->get_resolvers().begin(), f->get_resolvers().end(), [f](const auto &r)
                                   { return f == &r.get().get_flaw(); }));
                assert(std::none_of(f->get_resolvers().begin(), f->get_resolvers().end(), [this](const auto &r)
                                    { return value(static_cast<stresolver &>(r.get()).get_rho()) == utils::True; }));

                // we get the least expensive resolver..
                auto r = *std::min_element(f->get_resolvers().begin(), f->get_resolvers().end(), [](const auto &a, const auto &b)
                                           { return a.get().get_estimated_cost() < b.get().get_estimated_cost(); });
                assert(!is_infinite(r.get().get_estimated_cost()));
                set_current_resolver(r.get());

                // we apply the resolver..
                assume(static_cast<stresolver &>(r.get()).get_rho());
                STATE_CHANGED();

                set_current_resolver(std::nullopt);
                set_current_flaw(std::nullopt);

                check_graph();
            }
            // we solve all the current inconsistencies..
            solve_inconsistencies();
            check_graph();
        } while (!get_active_flaws().empty());
#endif
    }

    void solver::check_graph()
    {
        if (get_active_flaws().empty())
            return; // there are no more flaws to be solved..

        while (value(gamma) != utils::True) // we are building the initial causal graph (or we have explored the entire causal graph)..
        {
            switch (value(gamma))
            {
            case utils::Undefined:
                assume(utils::lit(gamma)); // (re)we enforce the pruning constraints..
                STATE_CHANGED();
                break;
            default:
                assert(decision_level() == 0);        // we must be at the root level..
                assert(value(gamma) == utils::False); // the gamma variable must be false..
                gamma = mk_var();                     // we create a new gamma variable for pruning the causal graph..
                LOG_DEBUG("γ: " + std::to_string(gamma));
                already_closed.clear();

                if (std::any_of(get_active_flaws().begin(), get_active_flaws().end(), [](const auto &f)
                                { return is_infinite(f->get_estimated_cost()); }))
                    build(); // we build the causal graph..
                else
                    add_layer(); // we add a layer to the graph..

                propagate(); // we propagate the constraints..

                visit_graph(); // we visit the causal graph..

                // we prune the causal graph..
                for (const auto &f : get_queued_flaws())
                    if (already_closed.insert(&f.get()).second) // we prune the flaw..
                        add_clause({utils::lit(gamma, false), !static_cast<stflaw &>(f.get()).get_phi()});
                propagate(); // we propagate the pruning constraints..

                assume(utils::lit(gamma)); // we enforce the pruning constraints..
                STATE_CHANGED();
                break;
            }
        }
    }

    void solver::visit_graph()
    {
        assert(std::none_of(get_active_flaws().begin(), get_active_flaws().end(), [](const auto &f)
                            { return is_infinite(f->get_estimated_cost()); }));

        // we try to negate the landmark candidates. if we cannot, then they are really landmarks..
        std::unordered_set<ratio::flaw *> c_lms;
        for (const auto &lm : landmark_candidates)
            if (lm->get_state() == utils::Undefined)
            { // we have to check if the landmark is really a landmark..
                assume(!static_cast<stflaw &>(*lm).get_phi());
                if (!get_decisions().empty())
                    semitone::pop(); // not a landmark, we backtrack..
                else
                    c_lms.insert(&*lm); // we have a landmark..
            }
            else
                c_lms.insert(&*lm); // we have a landmark..

        for (const auto &lm : c_lms)
            landmark_candidates.erase(lm); // we remove the landmarks from the candidates..

        visiting = true;

        // we visit the causal graph..
        struct state
        {
            std::size_t level;                   // the level of the flaw..
            flaw *f;                             // the flaw..
            std::unordered_set<resolver *> ress; // the resolvers of the flaw..
        };
        std::stack<state> stk;
        for (const auto &f : get_active_flaws())
        {
            state s = {0, &*f, {}};
            for (const auto &r : f->get_resolvers())
                if (value(static_cast<stresolver &>(r.get()).get_rho()) != utils::False)
                    s.ress.insert(&r.get());
            assert(!s.ress.empty());
            stk.push(std::move(s));
        }

        std::unordered_set<flaw *> to_expand;

        while (!stk.empty())
        {
            auto top = stk.top();
            stk.pop();

            if (value(static_cast<stflaw *>(top.f)->get_phi()) == utils::False)
                continue; // the flaw is unsolvable, we skip it..

            std::size_t c_level = get_decisions().size();
            // the current level can be higher than the level of the flaw, so we have to backtrack to the proper level..
            while (top.level < c_level)
            {
                semitone::pop(); // we backtrack to the current level..
                c_level = get_decisions().size();
            }

            set_current_flaw(*top.f);
            // we get the least expensive resolver..
            auto r = *std::min_element(top.ress.begin(), top.ress.end(), [](const auto &a, const auto &b)
                                       { return a->get_estimated_cost() < b->get_estimated_cost(); });
            set_current_resolver(*r);
            top.ress.erase(r); // we remove the resolver from the set of resolvers..
            assume(static_cast<stresolver &>(*r).get_rho());
            STATE_CHANGED();
            if (c_level + 1 == get_decisions().size())
            { // propagation succeeded..
                bool ok = true;
                for (const auto &f : get_active_flaws())
                    if (!f->is_expanded())
                    { // we have to expand the flaw..
                        ok = false;
                        to_expand.insert(&*f);
                    }
                if (!ok)
                {
                    semitone::pop(); // we backtrack..
                    set_current_resolver(std::nullopt);
                    set_current_flaw(std::nullopt);
                    continue;
                }

                std::vector<state> stk2;
                for (const auto &pre : r->get_preconditions())
                {
                    assert(value(static_cast<stflaw &>(pre.get()).get_phi()) == utils::True);
                    if (!pre.get().is_expanded())
                    { // we have to expand the precondition..
                        ok = false;
                        to_expand.insert(&pre.get());
                    }
                    else if (std::none_of(pre.get().get_resolvers().begin(), pre.get().get_resolvers().end(), [this](const auto &r)
                                          { return value(static_cast<stresolver &>(r.get()).get_rho()) == utils::True; }))
                    {
                        state s = {top.level + 1, &pre.get(), {}};
                        for (const auto &r : pre.get().get_resolvers())
                            if (value(static_cast<stresolver &>(r.get()).get_rho()) != utils::False)
                                s.ress.insert(&r.get());
                        assert(!s.ress.empty());
                        stk2.push_back(std::move(s));
                    }
                }
                if (ok && !stk2.empty()) // we have to visit the preconditions..
                    for (const auto &s : stk2)
                        stk.push(std::move(s));
                else // either we have no preconditions to visit or we have some preconditions that are not expanded, we backtrack..
                    semitone::pop();
            }
            else
            { // propagation failed..
                c_level = get_decisions().size();
                // the current level can be lower than the level of the flaw, so we remove states from the stack..
                while (!stk.empty() && stk.top().level > c_level)
                    stk.pop();
            }
            set_current_resolver(std::nullopt);
            set_current_flaw(std::nullopt);
        }

        while (!get_decisions().empty())
            semitone::pop(); // we backtrack to the root level..

        for (const auto &[n_r, c_r] : pending_mutexes)
            if (c_r->get_state() && get_active_flaws().count(&c_r->get_flaw())) // c_r has not been negated and it's flaw has not been solved..
            {
                set_current_flaw(c_r->get_flaw());
                // we check all the resolvers of the flaw..
                for (const auto &c_rs : c_r->get_flaw().get_resolvers())
                    if (c_r != &c_rs.get() && c_rs.get().get_state() == utils::Undefined)
                    {
                        set_current_resolver(c_rs.get());
                        assume(static_cast<stresolver &>(c_rs.get()).get_rho());
                        STATE_CHANGED();
                        if (get_decisions().size()) // propagation succeeded..
                            semitone::pop();        // we backtrack..
                    }
                for (const auto &c_rs : c_r->get_flaw().get_resolvers())
                    expand_flaw(new_flaw<mutex_flaw>(c_rs.get(), n_r->get_flaw()), true); // we create (and expand) the mutex flaws..
            }
        pending_mutexes.clear();

        // we expand the flaws..
        for (const auto &f : to_expand)
            expand_flaw(*f, true);

        visiting = false;
    }

    void solver::solve_inconsistencies()
    {
        LOG_DEBUG("[" << get_name() << "] Solving inconsistencies");

        std::vector<std::vector<std::pair<utils::lit, double>>> incs;
        std::queue<riddle::component_type *> q;
        for (const auto &tp : get_types())
            if (auto ct = dynamic_cast<riddle::component_type *>(tp.second.get()))
                q.push(ct);
        while (!q.empty())
        {
            auto tp = q.front();
            q.pop();
            for (const auto &etp : tp->get_types())
                if (auto ct = dynamic_cast<riddle::component_type *>(etp.second.get()))
                    q.push(ct);

            if (auto st_ct = dynamic_cast<stcomponent_type *>(tp)) // we have a flawable component type..
            {                                                      // we extract the current inconsistencies..
                auto st_incs = st_ct->get_current_incs();
                incs.insert(incs.end(), st_incs.begin(), st_incs.end());
            }
        };
        while (!incs.empty())
        {
            if (const auto &uns_inc = std::find_if(incs.cbegin(), incs.cend(), [](const auto &v)
                                                   { return v.empty(); });
                uns_inc != incs.cend())
            { // we have an unsolvable inconsistency..
                LOG_DEBUG("[" << get_name() << "] Dead end..");
                next(); // we move to the next state..
                STATE_CHANGED();
                if (value(gamma) != utils::True)
                    return; // we have to re-check the graph..
            }
            else
            { // we check if we have a trivial inconsistencies..
                std::vector<utils::lit> trivial;
                for (const auto &inc : incs)
                    if (inc.size() == 1)
                        trivial.push_back(inc.front().first);
                if (!trivial.empty()) // we have trivial inconsistencies..
                    for (const auto &l : trivial)
                    {
                        LOG_DEBUG("[" << get_name() << "] Trivial inconsistency: " << to_string(l));
                        assume(l);
                        STATE_CHANGED();
                        if (value(gamma) != utils::True)
                            return; // we have to re-check the graph..
                    }
                else
                { // we have a non-trivial inconsistencies, so we have to take a decision..
                    std::vector<std::pair<utils::lit, double>> bst_inc;
                    double k_inv = std::numeric_limits<double>::infinity();
                    for (const auto &inc : incs)
                    {
                        double bst_commit = std::numeric_limits<double>::infinity();
                        for ([[maybe_unused]] const auto &[choice, commit] : inc)
                            if (commit < bst_commit)
                                bst_commit = commit;
                        double c_k_inv = 0;
                        for ([[maybe_unused]] const auto &[choice, commit] : inc)
                            c_k_inv += 1l / (1l + (commit - bst_commit));
                        if (c_k_inv < k_inv)
                        {
                            k_inv = c_k_inv;
                            bst_inc = inc;
                        }
                    }

                    // we select the best choice (i.e. the least committing one) from those available for the best flaw..
                    auto l = std::min_element(bst_inc.cbegin(), bst_inc.cend(), [](const auto &ch0, const auto &ch1)
                                              { return ch0.second < ch1.second; })
                                 ->first;
                    LOG_DEBUG("[" << get_name() << "] Non-trivial inconsistency: " << to_string(l));
                    assume(l);
                    STATE_CHANGED();
                    if (value(gamma) != utils::True)
                        return; // we have to re-check the graph..
                }
            }

            incs.clear();
            for (const auto &tp : get_types())
                if (auto ct = dynamic_cast<riddle::component_type *>(tp.second.get()))
                    q.push(ct);
            while (!q.empty())
            {
                auto tp = q.front();
                q.pop();
                for (const auto &etp : tp->get_types())
                    if (auto ct = dynamic_cast<riddle::component_type *>(etp.second.get()))
                        q.push(ct);

                if (auto st_ct = dynamic_cast<stcomponent_type *>(tp)) // we have a timeline type..
                {                                                      // we extract the timeline..
                    auto st_incs = st_ct->get_current_incs();
                    incs.insert(incs.end(), st_incs.begin(), st_incs.end());
                }
            };
        }
    }
} // namespace ratio