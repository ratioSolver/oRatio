#include "stsolver.hpp"
#include "init.hpp"
#include "stflaws.hpp"
#include "sttypes.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <algorithm>
#include <cassert>

namespace ratio
{
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
        add_type(utils::make_u_ptr<ststate_variable>(*this));
        add_type(utils::make_u_ptr<streusable_resource>(*this));
        add_type(utils::make_u_ptr<stconsumable_resource>(*this));
    }

    riddle::bool_expr solver::new_bool() { return utils::make_s_ptr<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), utils::lit(mk_var())); }
    riddle::bool_expr solver::new_bool(const bool value)
    {
        auto l = value ? utils::TRUE_lit : utils::FALSE_lit;
        return utils::make_s_ptr<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::move(l));
    }
    utils::lbool solver::bool_value(const riddle::bool_term &expr) const noexcept { return value(static_cast<const riddle::bool_item &>(expr).get_lit()); }

    riddle::arith_expr solver::new_int() { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(mk_int(), utils::rational::one)); }
    riddle::arith_expr solver::new_int(const INT_TYPE value) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(utils::rational(value))); }
    riddle::arith_expr solver::new_int(const INT_TYPE lb, const INT_TYPE ub) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(mk_int(utils::rational(lb), utils::rational(ub)), utils::rational::one)); }
    riddle::arith_expr solver::new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(mk_int(utils::rational(lb), utils::rational(ub)), utils::rational::one)); }

    riddle::arith_expr solver::new_real() { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(mk_real(), utils::rational::one)); }
    riddle::arith_expr solver::new_real(utils::rational &&value) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(std::move(value))); }
    riddle::arith_expr solver::new_real(utils::rational &&lb, utils::rational &&ub) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(mk_real(std::move(lb), std::move(ub)), utils::rational::one)); }
    riddle::arith_expr solver::new_uncertain_real(utils::rational &&lb, utils::rational &&ub) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(mk_real(std::move(lb), std::move(ub)), utils::rational::one)); }

    riddle::arith_expr solver::new_time() { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), utils::lin(mk_tp(), utils::rational::one)); }
    riddle::arith_expr solver::new_time(utils::rational &&value) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), utils::lin(std::move(value))); }

    utils::inf_rational solver::arith_value(const riddle::arith_term &expr) const noexcept
    {
        if (expr.get_type().get_name() == riddle::int_kw || expr.get_type().get_name() == riddle::real_kw)
            return arith_val(static_cast<const riddle::arith_item &>(expr).get_lin());
        else
            return utils::inf_rational(tp_bounds(static_cast<const riddle::arith_item &>(expr).get_lin().vars.begin()->first).first);
    }

    riddle::string_expr solver::new_string() { return utils::make_s_ptr<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ""); }
    riddle::string_expr solver::new_string(std::string &&value) { return utils::make_s_ptr<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), std::move(value)); }
    std::string solver::string_value(const riddle::string_term &expr) const noexcept { return static_cast<const riddle::string_item &>(expr).get_string(); }

    riddle::enum_expr solver::new_enum(riddle::component_type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values)
    {
        std::vector<utils::ref_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        auto &ef = new_flaw<enum_flaw>(*this, std::move(causes), tp, std::move(values));
        return ef.get_var();
    }
    std::vector<utils::ref_wrapper<utils::enum_val>> solver::enum_value(const riddle::enum_term &expr) const noexcept
    {
        std::vector<utils::ref_wrapper<utils::enum_val>> dom;
        for (const auto &val : static_cast<const riddle::enum_item &>(expr).get_values())
            if (value(static_cast<const riddle::enum_item &>(expr).get_lit(*val)) != utils::False)
                dom.push_back(*val);
        return dom;
    }

    riddle::arith_expr solver::new_negation(riddle::arith_expr xpr)
    {
        if (xpr->get_type().get_name() == riddle::int_kw)
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), -static_cast<const riddle::arith_item &>(*xpr).get_lin());
        else if (xpr->get_type().get_name() == riddle::real_kw)
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), -static_cast<const riddle::arith_item &>(*xpr).get_lin());
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
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(sum));
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(sum));
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
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(sub));
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(sub));
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
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(prod));
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(prod));
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
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(div));
        else if (tp.get_name() == riddle::real_kw)
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(div));
        else
            throw std::runtime_error("Invalid type");
    }

    void solver::new_disjunction(std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<utils::ref_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        new_flaw<disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
    }

    void solver::new_clause(std::vector<riddle::bool_expr> &&exprs)
    {
        assert(!exprs.empty());
        std::vector<utils::lit> clause;
        for (const riddle::bool_expr &expr : exprs)
            if (auto b_xpr = utils::s_ptr_cast<riddle::bool_item>(expr))
                clause.push_back(b_xpr->get_lit());
            else if (auto n_xpr = utils::s_ptr_cast<riddle::bool_not>(expr))
            {
                if (auto b_xpr = utils::s_ptr_cast<riddle::bool_item>(n_xpr->get_arg()))
                    clause.push_back(!b_xpr->get_lit());
                else
                {
                    utils::lit p;
                    if (exprs.size() > 1) // we create a new variable for the constraint..
                        p = utils::lit(mk_var());
                    else if (get_current_resolver().has_value()) // we add the constraint to the current resolver..
                        p = static_cast<stresolver &>(*get_current_resolver().value()).get_rho();
                    else // we enforce the constraint directly..
                        p = utils::TRUE_lit;
                    clause.push_back(p);

                    if (auto lt_xpr = utils::s_ptr_cast<riddle::lt_term>(n_xpr->get_arg()))
                        add_ge(static_cast<riddle::arith_item &>(*lt_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*lt_xpr->get_rhs()).get_lin(), p);
                    else if (auto le_xpr = utils::s_ptr_cast<riddle::le_term>(n_xpr->get_arg()))
                        add_gt(static_cast<riddle::arith_item &>(*le_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*le_xpr->get_rhs()).get_lin(), p);
                    else if (auto eq_xpr = utils::s_ptr_cast<riddle::eq_term>(n_xpr->get_arg()))
                        make_neq(*eq_xpr->get_lhs(), *eq_xpr->get_rhs(), p);
                    else if (auto ge_xpr = utils::s_ptr_cast<riddle::ge_term>(n_xpr->get_arg()))
                        add_lt(static_cast<riddle::arith_item &>(*ge_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*ge_xpr->get_rhs()).get_lin(), p);
                    else if (auto gt_xpr = utils::s_ptr_cast<riddle::gt_term>(n_xpr->get_arg()))
                        add_le(static_cast<riddle::arith_item &>(*gt_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*gt_xpr->get_rhs()).get_lin(), p);
                    else
                        throw std::runtime_error("Invalid type");
                }
            }
            else
            {
                utils::lit p;
                if (exprs.size() > 1) // we create a new variable for the constraint..
                    p = utils::lit(mk_var());
                else if (get_current_resolver().has_value()) // we add the constraint to the current resolver..
                    p = static_cast<stresolver &>(*get_current_resolver().value()).get_rho();
                else // we enforce the constraint directly..
                    p = utils::TRUE_lit;
                clause.push_back(p);

                if (auto lt_xpr = utils::s_ptr_cast<riddle::lt_term>(expr))
                    add_lt(static_cast<riddle::arith_item &>(*lt_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*lt_xpr->get_rhs()).get_lin(), p);
                else if (auto le_xpr = utils::s_ptr_cast<riddle::le_term>(expr))
                    add_le(static_cast<riddle::arith_item &>(*le_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*le_xpr->get_rhs()).get_lin(), p);
                else if (auto eq_xpr = utils::s_ptr_cast<riddle::eq_term>(expr))
                    make_eq(*eq_xpr->get_lhs(), *eq_xpr->get_rhs(), p);
                else if (auto ge_xpr = utils::s_ptr_cast<riddle::ge_term>(expr))
                    add_ge(static_cast<riddle::arith_item &>(*ge_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*ge_xpr->get_rhs()).get_lin(), p);
                else if (auto gt_xpr = utils::s_ptr_cast<riddle::gt_term>(expr))
                    add_gt(static_cast<riddle::arith_item &>(*gt_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*gt_xpr->get_rhs()).get_lin(), p);
                else
                    throw std::runtime_error("Invalid type");
            }

        if (get_current_resolver().has_value())
            clause.push_back(!static_cast<stresolver &>(*get_current_resolver().value()).get_rho());
        add_clause(std::move(clause));
    }

    riddle::atom_expr solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args)
    {
        std::vector<utils::ref_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
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
        else if (auto lhs_xpr = dynamic_cast<riddle::arith_item *>(&lhs)) // we are dealing with an arithmetic constraint..
            return arith_lb(lhs_xpr->get_lin()) <= arith_ub(static_cast<riddle::arith_item &>(rhs).get_lin()) && arith_ub(lhs_xpr->get_lin()) >= arith_lb(static_cast<riddle::arith_item &>(rhs).get_lin());
        else if (auto lhs_xpr = dynamic_cast<riddle::bool_item *>(&lhs)) // we are dealing with a boolean constraint..
            return value(lhs_xpr->get_lit()) == value(static_cast<riddle::bool_item &>(rhs).get_lit()) || value(lhs_xpr->get_lit()) == utils::Undefined || value(static_cast<riddle::bool_item &>(rhs).get_lit()) == utils::Undefined;
        else if (auto lhs_xpr = dynamic_cast<riddle::string_item *>(&lhs)) // we are dealing with a string constraint..
            return lhs_xpr->get_string() == static_cast<riddle::string_item &>(rhs).get_string();
        else if (auto lhs_xpr = dynamic_cast<riddle::enum_item *>(&lhs))
        { // we are dealing with an enumeration constraint..
            if (auto rhs_xpr = dynamic_cast<riddle::enum_item *>(&rhs))
            {
                // we compute the intersection of the two domains
                std::unordered_set<utils::enum_val *> intersection;
                for (const auto &v : lhs_xpr->get_values())
                    for (const auto &w : rhs_xpr->get_values())
                        if (v == w)
                        {
                            intersection.insert(&*v);
                            break;
                        }
                return std::any_of(intersection.begin(), intersection.end(), [&](const utils::enum_val *v)
                                   { return value(lhs_xpr->get_lit(*v)) == value(rhs_xpr->get_lit(*v)) || value(lhs_xpr->get_lit(*v)) == utils::Undefined || value(rhs_xpr->get_lit(*v)) == utils::Undefined; });
            }
            else
            {
                for (const auto &v : lhs_xpr->get_values())
                    if (match(*lhs_xpr, static_cast<riddle::term &>(*v)))
                        return true;
                return false;
            }
        }
        else if (auto lhs_xpr = dynamic_cast<riddle::atom_term *>(&lhs))
        { // we are dealing with atoms..
            auto rhs_xpr = static_cast<riddle::atom_term *>(&rhs);
            std::queue<riddle::predicate *> q;
            q.push(static_cast<riddle::predicate *>(&lhs_xpr->get_type()));
            while (!q.empty())
            {
                for (const auto &[f_name, f] : q.front()->get_fields())
                    if (!f->is_synthetic() && !match(*lhs_xpr->get(f_name), *rhs_xpr->get(f_name)))
                        return false;
                for (const auto &pp : q.front()->get_parents())
                    q.push(&*pp);
                q.pop();
            }
            return true;
        }
        else if (auto lhs_xpr = dynamic_cast<riddle::component *>(&lhs))
        { // we are dealing with components..
            auto rhs_xpr = static_cast<riddle::component *>(&rhs);
            std::queue<riddle::component_type *> q;
            q.push(static_cast<riddle::component_type *>(&lhs_xpr->get_type()));
            while (!q.empty())
            {
                for (const auto &[f_name, f] : q.front()->get_fields())
                    if (!f->is_synthetic() && !match(*lhs_xpr->get(f_name), *rhs_xpr->get(f_name)))
                        return false;
                for (const auto &pp : q.front()->get_parents())
                    q.push(&*pp);
                q.pop();
            }
            return true;
        }
        else
            throw std::runtime_error("Invalid type");
    }

    void solver::make_eq(riddle::term &lhs, riddle::term &rhs, const utils::lit &p)
    {
        if (&lhs.get_type() != &rhs.get_type()) // the types are different, so the constraint is always false..
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
                        if (v == w)
                        {
                            intersection.insert(&*v);
                            break;
                        }
                if (intersection.empty())
                    add_clause({!p}); // the domains are disjoint, so the constraint is always false..

                // the values outside the intersection are pruned if the equality control variable becomes true..
                for (const auto &v : lhs_xpr->get_values())
                    if (!intersection.count(&*v))
                        add_clause({!lhs_xpr->get_lit(*v), !p});
                for (const auto &v : rhs_xpr->get_values())
                    if (!intersection.count(&*v))
                        add_clause({!rhs_xpr->get_lit(*v), !p});

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
                    if (&*v != &*static_cast<utils::enum_val *>(&rhs))
                        add_clause({!p, !lhs_xpr->get_lit(*v)});
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
                    if (!f->is_synthetic())
                        make_eq(*lhs_xpr->get(f_name), *rhs_xpr->get(f_name), p);
                for (const auto &pp : q.front()->get_parents())
                    q.push(&*pp);
                q.pop();
            }
        }
        else if (auto lhs_xpr = dynamic_cast<riddle::component *>(&lhs))
        { // we are dealing with components..
            auto rhs_xpr = static_cast<riddle::component *>(&rhs);
            std::queue<riddle::component_type *> q;
            q.push(static_cast<riddle::component_type *>(&lhs_xpr->get_type()));
            while (!q.empty())
            {
                for (const auto &[f_name, f] : q.front()->get_fields())
                    if (!f->is_synthetic())
                        make_eq(*lhs_xpr->get(f_name), *rhs_xpr->get(f_name), p);
                for (const auto &pp : q.front()->get_parents())
                    q.push(&*pp);
                q.pop();
            }
        }
        else
            throw std::runtime_error("Invalid type");
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
                        if (v == w)
                        { // choosing a value from one domain excludes the corresponding value from the other domain..
                            add_clause({!p, !lhs_ei_xpr->get_lit(*v), !rhs_ei_xpr->get_lit(*v)});
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
                    if (!f->is_synthetic())
                    {
                        auto neq = utils::lit(mk_var());
                        make_neq(*lhs_at_xpr->get(f_name), *rhs_at_xpr->get(f_name), neq);
                        clause.push_back(neq);
                    }
                for (const auto &pp : q.front()->get_parents())
                    q.push(&*pp);
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
                    if (!f->is_synthetic())
                    {
                        auto neq = utils::lit(mk_var());
                        make_neq(*lhs_c_xpr->get(f_name), *rhs_c_xpr->get(f_name), neq);
                        clause.push_back(neq);
                    }
                for (const auto &pp : q.front()->get_parents())
                    q.push(&*pp);
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
            // we get the most expensive flaw..
            auto f = *std::max_element(get_active_flaws().begin(), get_active_flaws().end(), [](const auto &a, const auto &b)
                                       { return a->get_estimated_cost() < b->get_estimated_cost(); });
            set_current_flaw(*f);

            if (is_infinite(f->get_estimated_cost()))
            { // we don't know how to solve this flaw :(
                do
                { // we have to search..
                    next();
                    STATE_CHANGED();
                    check_graph();
                } while (std::any_of(get_active_flaws().begin(), get_active_flaws().end(), [](const auto &f)
                                     { return is_infinite(f->get_estimated_cost()); }));
                continue;
            }

            // we get the least expensive resolver..
            auto r = *std::min_element(f->get_resolvers().begin(), f->get_resolvers().end(), [](const auto &a, const auto &b)
                                       { return a->get_estimated_cost() < b->get_estimated_cost(); });
            set_current_resolver(*r);

            // we apply the resolver..
            assume(static_cast<stresolver &>(*r).get_rho());
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
                // we get the most expensive flaw..
                auto f = *std::max_element(get_active_flaws().begin(), get_active_flaws().end(), [](const auto &a, const auto &b)
                                           { return a->get_estimated_cost() < b->get_estimated_cost(); });
                set_current_flaw(*f);

                if (is_infinite(f->get_estimated_cost()))
                { // we don't know how to solve this flaw :(
                    do
                    { // we have to search..
                        next();
                        STATE_CHANGED();
                        check_graph();
                    } while (std::any_of(get_active_flaws().begin(), get_active_flaws().end(), [](const auto &f)
                                         { return is_infinite(f->get_estimated_cost()); }));
                    continue;
                }

                // we get the least expensive resolver..
                auto r = *std::min_element(f->get_resolvers().begin(), f->get_resolvers().end(), [](const auto &a, const auto &b)
                                           { return a->get_estimated_cost() < b->get_estimated_cost(); });
                set_current_resolver(*r);

                // we apply the resolver..
                assume(static_cast<stresolver &>(*r).get_rho());
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
                assert(decision_level() == 0); // we must be at the root level..
                gamma = mk_var();              // we create a new gamma variable for pruning the causal graph..
                LOG_DEBUG("γ: " + std::to_string(gamma));
                already_closed.clear();

                if (std::any_of(get_active_flaws().begin(), get_active_flaws().end(), [](const auto &f)
                                { return is_infinite(f->get_estimated_cost()); }))
                    build(); // we build the causal graph..
                else
                    add_layer(); // we add a layer to the graph..

                propagate(); // we propagate the constraints..

                // we prune the causal graph..
                for (const auto &f : get_queued_flaws())
                    if (already_closed.insert(&*f).second) // we prune the flaw..
                        add_clause({utils::lit(gamma, false), !static_cast<stflaw &>(*f).get_phi()});
                propagate(); // we propagate the pruning constraints..

                assume(utils::lit(gamma)); // we enforce the pruning constraints..
                STATE_CHANGED();
                break;
            }
        }
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

            if (auto st_ct = dynamic_cast<stcomponent_type *>(tp)) // we have a timeline type..
            {                                                      // we extract the timeline..
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