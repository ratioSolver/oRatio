#include "stsolver.hpp"
#include "stflaws.hpp"
#include "sttypes.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <algorithm>
#include <cassert>

namespace ratio
{
    stsolver::stsolver(std::string_view name) noexcept : graph(name) {}

    riddle::bool_expr stsolver::new_bool() { return utils::make_s_ptr<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), utils::lit(net.new_var())); }
    riddle::bool_expr stsolver::new_bool(const bool value)
    {
        auto l = value ? utils::TRUE_lit : utils::FALSE_lit;
        return utils::make_s_ptr<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::move(l));
    }
    utils::lbool stsolver::bool_value(const riddle::bool_term &expr) const noexcept { return net.value(static_cast<const riddle::bool_item &>(expr).get_lit()); }

    riddle::arith_expr stsolver::new_int() { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(net.new_int(), utils::rational::one)); }
    riddle::arith_expr stsolver::new_int(const INT_TYPE value) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(utils::rational(value))); }
    riddle::arith_expr stsolver::new_int(const INT_TYPE lb, const INT_TYPE ub) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(net.new_int(utils::rational(lb), utils::rational(ub)), utils::rational::one)); }
    riddle::arith_expr stsolver::new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(net.new_int(utils::rational(lb), utils::rational(ub)), utils::rational::one)); }

    riddle::arith_expr stsolver::new_real() { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(net.new_real(), utils::rational::one)); }
    riddle::arith_expr stsolver::new_real(utils::rational &&value) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(std::move(value))); }
    riddle::arith_expr stsolver::new_real(utils::rational &&lb, utils::rational &&ub) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(net.new_real(std::move(lb), std::move(ub)), utils::rational::one)); }
    riddle::arith_expr stsolver::new_uncertain_real(utils::rational &&lb, utils::rational &&ub) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(net.new_real(std::move(lb), std::move(ub)), utils::rational::one)); }

    riddle::arith_expr stsolver::new_time() { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), utils::lin(net.new_tp(), utils::rational::one)); }
    riddle::arith_expr stsolver::new_time(utils::rational &&value) { return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), utils::lin(std::move(value))); }

    utils::inf_rational stsolver::arith_value(const riddle::arith_term &expr) const noexcept
    {
        if (expr.get_type().get_name() == riddle::int_kw || expr.get_type().get_name() == riddle::real_kw)
            return net.arith_value(static_cast<const riddle::arith_item &>(expr).get_lin());
        else
            return utils::inf_rational(net.tp_bounds(static_cast<const riddle::arith_item &>(expr).get_lin().vars.begin()->first).first);
    }

    riddle::string_expr stsolver::new_string() { return utils::make_s_ptr<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ""); }
    riddle::string_expr stsolver::new_string(std::string &&value) { return utils::make_s_ptr<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), std::move(value)); }
    std::string stsolver::string_value(const riddle::string_term &expr) const noexcept { return static_cast<const riddle::string_item &>(expr).get_string(); }

    riddle::enum_expr stsolver::new_enum(riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values)
    {
        std::vector<utils::ref_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        auto &af = new_flaw<stenum_flaw>(*this, std::move(causes), tp, std::move(values));
        return af.get_var();
    }
    std::vector<utils::ref_wrapper<utils::enum_val>> stsolver::enum_value(const riddle::enum_term &expr) const noexcept
    {
        std::vector<utils::ref_wrapper<utils::enum_val>> dom;
        for (const auto &val : static_cast<const riddle::enum_item &>(expr).get_values())
            if (net.value(static_cast<const riddle::enum_item &>(expr).get_lit(*val)) != utils::False)
                dom.push_back(*val);
        return dom;
    }

    riddle::arith_expr stsolver::new_negation(riddle::arith_expr xpr)
    {
        if (xpr->get_type().get_name() == riddle::int_kw)
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), -static_cast<const riddle::arith_item &>(*xpr).get_lin());
        else if (xpr->get_type().get_name() == riddle::real_kw)
            return utils::make_s_ptr<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), -static_cast<const riddle::arith_item &>(*xpr).get_lin());
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr stsolver::new_sum(std::vector<riddle::arith_expr> &&xprs)
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

    riddle::arith_expr stsolver::new_subtraction(std::vector<riddle::arith_expr> &&xprs)
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

    riddle::arith_expr stsolver::new_product(std::vector<riddle::arith_expr> &&xprs)
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

    riddle::arith_expr stsolver::new_division(std::vector<riddle::arith_expr> &&xprs)
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

    void stsolver::new_disjunction(std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<utils::ref_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        new_flaw<stdisjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
    }

    void stsolver::new_clause(std::vector<riddle::bool_expr> &&exprs)
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
                    throw std::runtime_error("Invalid type");
            }
            else
            {
                utils::lit p;
                if (exprs.size() > 1) // we create a new variable for the constraint..
                    p = utils::lit(net.new_var());
                else if (get_current_resolver().has_value()) // we add the constraint to the current resolver..
                    p = static_cast<stresolver &>(*get_current_resolver().value()).get_rho();
                else // we enforce the constraint directly..
                    p = utils::TRUE_lit;
                clause.push_back(p);

                if (auto lt_xpr = utils::s_ptr_cast<riddle::lt_term>(expr))
                    net.new_lt(static_cast<riddle::arith_item &>(*lt_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*lt_xpr->get_rhs()).get_lin(), p);
                else if (auto le_xpr = utils::s_ptr_cast<riddle::le_term>(expr))
                    net.new_le(static_cast<riddle::arith_item &>(*le_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*le_xpr->get_rhs()).get_lin(), p);
                else if (auto eq_xpr = utils::s_ptr_cast<riddle::eq_term>(expr))
                    make_eq(*eq_xpr->get_lhs(), *eq_xpr->get_rhs(), p);
                else if (auto ge_xpr = utils::s_ptr_cast<riddle::ge_term>(expr))
                    net.new_ge(static_cast<riddle::arith_item &>(*ge_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*ge_xpr->get_rhs()).get_lin(), p);
                else if (auto gt_xpr = utils::s_ptr_cast<riddle::gt_term>(expr))
                    net.new_gt(static_cast<riddle::arith_item &>(*gt_xpr->get_lhs()).get_lin(), static_cast<riddle::arith_item &>(*gt_xpr->get_rhs()).get_lin(), p);
                else
                    throw std::runtime_error("Invalid type");
            }

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

    void stsolver::make_eq(riddle::term &lhs, riddle::term &rhs, const utils::lit &p)
    {
        if (&lhs.get_type() != &rhs.get_type()) // the types are different, so the constraint is always false..
            net.new_clause({!p});
        else if (auto lhs_xpr = dynamic_cast<riddle::arith_item *>(&lhs)) // we are dealing with an arithmetic constraint..
            net.new_eq(lhs_xpr->get_lin(), static_cast<riddle::arith_item *>(&rhs)->get_lin(), p);
        else if (auto lhs_xpr = dynamic_cast<riddle::bool_item *>(&lhs)) // we are dealing with a boolean constraint..
        {
            auto rhs_xpr = static_cast<riddle::bool_item *>(&rhs);
            net.new_clause({!p, lhs_xpr->get_lit(), !rhs_xpr->get_lit()});
            net.new_clause({!p, !lhs_xpr->get_lit(), rhs_xpr->get_lit()});
            net.new_clause({p, lhs_xpr->get_lit(), rhs_xpr->get_lit()});
        }
        else if (auto lhs_xpr = dynamic_cast<riddle::string_item *>(&lhs)) // we are dealing with a string constraint..
        {
            if (lhs_xpr->get_string() != static_cast<riddle::string_item *>(&rhs)->get_string()) // the strings are different, so the constraint is always false..
                net.new_clause({!p});
        }
        else if (auto lhs_xpr = dynamic_cast<riddle::enum_item *>(&lhs)) // we are dealing with an enumeration constraint..
        {
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
                    net.new_clause({!p}); // the domains are disjoint, so the constraint is always false..

                // the values outside the intersection are pruned if the equality control variable becomes true..
                for (const auto &v : lhs_xpr->get_values())
                    if (!intersection.count(&*v))
                        net.new_clause({!lhs_xpr->get_lit(*v), !p});
                for (const auto &v : rhs_xpr->get_values())
                    if (!intersection.count(&*v))
                        net.new_clause({!rhs_xpr->get_lit(*v), !p});

                for (const auto &v : intersection)
                {
                    net.new_clause({!p, lhs_xpr->get_lit(*v), !rhs_xpr->get_lit(*v)});
                    net.new_clause({!p, !lhs_xpr->get_lit(*v), rhs_xpr->get_lit(*v)});
                }
            }
            else
            {
                net.new_clause({!p, lhs_xpr->get_lit(*static_cast<utils::enum_val *>(&rhs))});
                for (const auto &v : lhs_xpr->get_values())
                    if (&*v != &*static_cast<utils::enum_val *>(&rhs))
                        net.new_clause({!p, !lhs_xpr->get_lit(*v)});
            }
        }
        else if (auto rhs_xpr = dynamic_cast<riddle::enum_item *>(&rhs)) // we are dealing with an enumeration constraint..
            make_eq(*rhs_xpr, lhs, p);
        else if (auto lhs_xpr = dynamic_cast<riddle::atom_term *>(&lhs)) // we are dealing with atoms..
        {
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
        else if (auto lhs_xpr = dynamic_cast<riddle::component *>(&lhs)) // we are dealing with components..
        {
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

    bool stsolver::solve()
    {
        net.propagate();
        build(); // we build the causal graph..

        while (true)
        { // we try to solve the problem with the current causal graph..

            // we get the most expensive flaw..
            auto f = *std::max_element(active_flaws.begin(), active_flaws.end(), [](const auto &a, const auto &b)
                                       { return a->get_estimated_cost() < b->get_estimated_cost(); });
            // we get the least expensive resolver..
            auto r = *std::min_element(f->get_resolvers().begin(), f->get_resolvers().end(), [](const auto &a, const auto &b)
                                       { return a->get_estimated_cost() < b->get_estimated_cost(); });

            // we apply the resolver..
            net.assume(static_cast<stresolver &>(*r).get_rho());
        }
    }
} // namespace ratio