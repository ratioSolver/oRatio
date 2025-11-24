#include "solver_core.hpp"
#include "items.hpp"
#include "logging.hpp"
#include <cassert>

#ifdef ORATIO_ENABLE_LISTENERS
#define CURRENT_FLAW(f) current_flaw(f)
#define CURRENT_RESOLVER(r) current_resolver(r)
#define NEW_CAUSAL_LINK(f, r) causal_link_added(f, r)
#else
#define CURRENT_FLAW(f)
#define CURRENT_RESOLVER(r)
#define NEW_CAUSAL_LINK(f, r)
#endif

namespace ratio
{
    flaw::flaw(solver_core &slv, std::vector<std::reference_wrapper<resolver>> &&causes) noexcept : slv(slv), causes(std::move(causes))
    {
        for (auto &c : this->causes)
            c.get().preconditions.push_back(*this);
    }
    linspire::solver &flaw::get_lin() const noexcept { return slv.lin_slv; }
    arc_consistency::solver &flaw::get_ac() const noexcept { return slv.ac_slv; }

    json::json flaw::to_json() const
    {
        json::json j_flaw;
        if (!causes.empty())
        {
            json::json j_causes(json::json_type::array);
            for (const auto &c : causes)
                j_causes.push_back(c.get().get_id());
            j_flaw["causes"] = std::move(j_causes);
        }
        return j_flaw;
    }

    resolver::resolver(flaw &flw, utils::rational &&intrinsic_cost) noexcept : flw(flw), intrinsic_cost(std::move(intrinsic_cost)) { flw.resolvers.push_back(*this); }
    void resolver::execute(const riddle::bool_expr &expr)
    {
        if (!get_solver().execute(expr))
            throw std::runtime_error("Failed to execute expression in resolver");
    }

    json::json resolver::to_json() const
    {
        json::json j_resolver{{"flaw", flw.get_id()}, {"intrinsic_cost", linspire::to_json(intrinsic_cost)}};
        if (!preconditions.empty())
        {
            json::json j_preconditions(json::json_type::array);
            for (const auto &p : preconditions)
                j_preconditions.push_back(p.get().get_id());
            j_resolver["preconditions"] = std::move(j_preconditions);
        }
        return j_resolver;
    }

    solver_core::solver_core(std::string_view name) noexcept : riddle::core(name) {}

    riddle::bool_expr solver_core::new_bool() { return std::make_shared<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), ac_slv.new_sat()); }
    riddle::bool_expr solver_core::new_bool(const bool value)
    {
        auto l = value ? utils::TRUE_lit : utils::FALSE_lit;
        return std::make_shared<riddle::bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::move(l));
    }
    utils::lbool solver_core::bool_value(const riddle::bool_term &expr) const noexcept { return ac_slv.sat_val(static_cast<const riddle::bool_item &>(expr).get_lit()); }

    riddle::arith_expr solver_core::new_int() { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::lin(lin_slv.new_var(), utils::rational::one)); }
    riddle::arith_expr solver_core::new_int(const INT_TYPE value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), utils::rational(value)); }
    riddle::arith_expr solver_core::new_int(const INT_TYPE lb, const INT_TYPE ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), lin_slv.new_var(utils::rational(lb), utils::rational(ub))); }
    riddle::arith_expr solver_core::new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), lin_slv.new_var(utils::rational(lb), utils::rational(ub))); }

    riddle::arith_expr solver_core::new_real() { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), utils::lin(lin_slv.new_var(), utils::rational::one)); }
    riddle::arith_expr solver_core::new_real(utils::rational &&value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(value)); }
    riddle::arith_expr solver_core::new_real(utils::rational &&lb, utils::rational &&ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), lin_slv.new_var(std::move(lb), std::move(ub))); }
    riddle::arith_expr solver_core::new_uncertain_real(utils::rational &&lb, utils::rational &&ub) { return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), lin_slv.new_var(std::move(lb), std::move(ub))); }

    riddle::arith_expr solver_core::new_time() { return std::make_shared<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), utils::lin(lin_slv.new_var(), utils::rational::one)); }
    riddle::arith_expr solver_core::new_time(utils::rational &&value) { return std::make_shared<riddle::arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), std::move(value)); }

    utils::inf_rational solver_core::arith_value(const riddle::arith_term &expr) const noexcept { return lin_slv.val(static_cast<const riddle::arith_item &>(expr).get_lin()); }

    riddle::string_expr solver_core::new_string() { return std::make_shared<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ""); }
    riddle::string_expr solver_core::new_string(std::string &&value) { return std::make_shared<riddle::string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), std::move(value)); }
    std::string solver_core::string_value(const riddle::string_term &expr) const noexcept { return static_cast<const riddle::string_item &>(expr).get_string(); }

    std::vector<riddle::expr> solver_core::enum_value(const riddle::enum_term &expr) const noexcept
    {
        auto &dom = ac_slv.domain(static_cast<const riddle::enum_item &>(expr).get_var());
        std::vector<riddle::expr> values;
        for (auto ev_ptr : static_cast<const riddle::enum_item &>(expr).get_values())
            if (dom.find(&*ev_ptr) != dom.end())
                values.push_back(ev_ptr);
        return values;
    };

    riddle::arith_expr solver_core::new_negation(riddle::arith_expr xpr)
    {
        if (xpr->get_type().get_name() == riddle::int_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), -static_cast<const riddle::arith_item &>(*xpr).get_lin());
        else if (xpr->get_type().get_name() == riddle::real_kw)
            return std::make_shared<riddle::arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), -static_cast<const riddle::arith_item &>(*xpr).get_lin());
        else
            throw std::runtime_error("Invalid type");
    }

    riddle::arith_expr solver_core::new_sum(std::vector<riddle::arith_expr> &&xprs)
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
    riddle::arith_expr solver_core::new_subtraction(std::vector<riddle::arith_expr> &&xprs)
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
    riddle::arith_expr solver_core::new_product(std::vector<riddle::arith_expr> &&xprs)
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
    riddle::arith_expr solver_core::new_division(std::vector<riddle::arith_expr> &&xprs)
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

    void solver_core::add_causal_link(flaw &f, resolver &r) noexcept
    {
        f.supports.push_back(r);
        r.preconditions.push_back(f);
        NEW_CAUSAL_LINK(f, r);
    }

    bool solver_core::match(riddle::term &lhs, riddle::term &rhs) const
    {
        if (&lhs == &rhs) // the terms are the same, so they match..
            return true;
        else if (&lhs.get_type() != &rhs.get_type()) // the types are different, so the terms cannot match..
            return false;
        else if (auto lhs_xpr = dynamic_cast<riddle::arith_item *>(&lhs)) // we are dealing with arithmetic terms..
            return lin_slv.match(lhs_xpr->get_lin(), static_cast<riddle::arith_item &>(rhs).get_lin());
        else if (auto lhs_bxpr = dynamic_cast<riddle::bool_item *>(&lhs)) // we are dealing with boolean terms..
            return ac_slv.match(lhs_bxpr->get_lit(), static_cast<riddle::bool_item &>(rhs).get_lit());
        else if (auto lhs_enum_xpr = dynamic_cast<riddle::enum_item *>(&lhs)) // we are dealing with enum terms..
        {
            if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(&rhs))
                return ac_slv.match(lhs_enum_xpr->get_var(), rhs_enum_xpr->get_var());
            else
                return ac_slv.allows(lhs_enum_xpr->get_var(), rhs);
        }
        else if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(&rhs)) // we are dealing with enum terms..
            return ac_slv.allows(rhs_enum_xpr->get_var(), lhs);
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

    bool solver_core::execute(const riddle::bool_expr &expr) noexcept
    {
        if (auto n_xpr = dynamic_cast<riddle::bool_not *>(expr.get()))
        {
            if (auto b_xpr = dynamic_cast<riddle::bool_item *>(n_xpr->get_arg().get()))
            {
                add_constraint(ac_slv.new_assign(utils::variable(b_xpr->get_lit()), utils::sign(b_xpr->get_lit()) ? arc_consistency::solver::False : arc_consistency::solver::True));
                return true;
            }
            else if (auto lt_xpr = dynamic_cast<riddle::lt_term *>(n_xpr->get_arg().get()))
                return lin_slv.new_gt(static_cast<riddle::arith_item *>(lt_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(lt_xpr->get_rhs().get())->get_lin(), false, c_res ? std::make_optional(std::ref(c_res->get().cnst)) : std::nullopt);
            else if (auto le_xpr = dynamic_cast<riddle::le_term *>(n_xpr->get_arg().get()))
                return lin_slv.new_gt(static_cast<riddle::arith_item *>(le_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(le_xpr->get_rhs().get())->get_lin(), true, c_res ? std::make_optional(std::ref(c_res->get().cnst)) : std::nullopt);
            else if (auto eq_xpr = dynamic_cast<riddle::eq_term *>(n_xpr->get_arg().get()))
            {
                if (&*eq_xpr->get_lhs() == &*eq_xpr->get_rhs()) // the terms are the same, so they are equal..
                    return false;
                else if (&eq_xpr->get_lhs()->get_type() != &eq_xpr->get_lhs()->get_type()) // the types are different, so the constraint is always false..
                    return true;
                else if (auto lhs_xpr = std::dynamic_pointer_cast<riddle::arith_item>(eq_xpr->get_lhs())) // we are dealing with an arithmetic constraint..
                {
                    new_clause({std::make_shared<riddle::lt_term>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), lhs_xpr, std::static_pointer_cast<riddle::arith_item>(eq_xpr->get_rhs())), std::make_shared<riddle::gt_term>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), lhs_xpr, std::static_pointer_cast<riddle::arith_item>(eq_xpr->get_rhs()))});
                    return true;
                }
                else if (auto lhs_sxpr = dynamic_cast<riddle::string_item *>(eq_xpr->get_lhs().get())) // we are dealing with a string constraint..
                    return lhs_sxpr->get_string() != static_cast<riddle::string_item &>(*eq_xpr->get_rhs()).get_string();
                else if (auto lhs_bxpr = dynamic_cast<riddle::bool_item *>(eq_xpr->get_lhs().get())) // we are dealing with a boolean constraint..
                {
                    add_constraint(ac_slv.new_distinct(utils::variable(lhs_bxpr->get_lit()), utils::variable(static_cast<riddle::bool_item &>(*eq_xpr->get_rhs()).get_lit())));
                    return true;
                }
                else if (auto lhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_lhs().get())) // we are dealing with an enum constraint..
                {
                    if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                    { // both sides are enum items..
                        add_constraint(ac_slv.new_distinct(lhs_enum_xpr->get_var(), rhs_enum_xpr->get_var()));
                        return true;
                    }
                    else
                    {
                        add_constraint(ac_slv.new_forbid(lhs_enum_xpr->get_var(), *eq_xpr->get_rhs()));
                        return true;
                    }
                }
                else if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                {
                    add_constraint(ac_slv.new_forbid(rhs_enum_xpr->get_var(), *eq_xpr->get_lhs()));
                    return true;
                }
                else if (auto lhs_atm = dynamic_cast<riddle::atom_term *>(eq_xpr->get_lhs().get()))
                {
                    auto rhs_atm = static_cast<riddle::atom_term *>(eq_xpr->get_rhs().get());
                    std::vector<riddle::bool_expr> clause_exprs;
                    std::queue<riddle::predicate *> q;
                    q.push(static_cast<riddle::predicate *>(&lhs_xpr->get_type()));
                    while (!q.empty())
                    {
                        for (const auto &[f_name, f] : q.front()->get_fields())
                            clause_exprs.push_back(std::make_shared<riddle::bool_not>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), std::make_shared<riddle::eq_term>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), lhs_atm->get(f_name), rhs_atm->get(f_name))));
                        for (const auto &pp : q.front()->get_parents())
                            q.push(&pp.get());
                        q.pop();
                    }
                    new_clause(std::move(clause_exprs));
                    return true;
                }
                else
                    return true;
            }
            else if (auto ge_xpr = dynamic_cast<riddle::ge_term *>(n_xpr->get_arg().get()))
                return lin_slv.new_lt(static_cast<riddle::arith_item *>(ge_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(ge_xpr->get_rhs().get())->get_lin(), false, c_res ? std::make_optional(std::ref(c_res->get().cnst)) : std::nullopt);
            else if (auto gt_xpr = dynamic_cast<riddle::gt_term *>(n_xpr->get_arg().get()))
                return lin_slv.new_lt(static_cast<riddle::arith_item *>(gt_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(gt_xpr->get_rhs().get())->get_lin(), true, c_res ? std::make_optional(std::ref(c_res->get().cnst)) : std::nullopt);
            else
                return false; // unknown expression inside negation..
        }
        else
        {
            if (auto b_xpr = dynamic_cast<riddle::bool_item *>(expr.get()))
            {
                add_constraint(ac_slv.new_assign(utils::variable(b_xpr->get_lit()), utils::sign(b_xpr->get_lit()) ? arc_consistency::solver::True : arc_consistency::solver::False));
                return true;
            }
            else if (auto lt_xpr = dynamic_cast<riddle::lt_term *>(expr.get()))
                return lin_slv.new_lt(static_cast<riddle::arith_item *>(lt_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(lt_xpr->get_rhs().get())->get_lin(), true, c_res ? std::make_optional(std::ref(c_res->get().cnst)) : std::nullopt);
            else if (auto le_xpr = dynamic_cast<riddle::le_term *>(expr.get()))
                return lin_slv.new_lt(static_cast<riddle::arith_item *>(le_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(le_xpr->get_rhs().get())->get_lin(), false, c_res ? std::make_optional(std::ref(c_res->get().cnst)) : std::nullopt);
            else if (auto eq_xpr = dynamic_cast<riddle::eq_term *>(expr.get()))
            {
                if (&*eq_xpr->get_lhs() == &*eq_xpr->get_rhs()) // the terms are the same, so they are equal..
                    return true;
                else if (&eq_xpr->get_lhs()->get_type() != &eq_xpr->get_lhs()->get_type()) // the types are different, so the constraint is always false..
                    return false;
                else if (auto lhs_xpr = dynamic_cast<riddle::arith_item *>(eq_xpr->get_lhs().get())) // we are dealing with an arithmetic constraint..
                    return lin_slv.new_eq(lhs_xpr->get_lin(), static_cast<riddle::arith_item *>(eq_xpr->get_rhs().get())->get_lin(), c_res ? std::make_optional(std::ref(c_res->get().cnst)) : std::nullopt);
                else if (auto lhs_sxpr = dynamic_cast<riddle::string_item *>(eq_xpr->get_lhs().get())) // we are dealing with a string constraint..
                    return lhs_sxpr->get_string() == static_cast<riddle::string_item &>(*eq_xpr->get_rhs()).get_string();
                else if (auto lhs_bxpr = dynamic_cast<riddle::bool_item *>(eq_xpr->get_lhs().get())) // we are dealing with a boolean constraint..
                {
                    add_constraint(ac_slv.new_equal(utils::variable(lhs_bxpr->get_lit()), utils::variable(static_cast<riddle::bool_item &>(*eq_xpr->get_rhs()).get_lit())));
                    return true;
                }
                else if (auto lhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_lhs().get())) // we are dealing with an enum constraint..
                {
                    if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                    { // both sides are enum items..
                        add_constraint(ac_slv.new_equal(lhs_enum_xpr->get_var(), rhs_enum_xpr->get_var()));
                        return true;
                    }
                    else
                    {
                        add_constraint(ac_slv.new_assign(lhs_enum_xpr->get_var(), *eq_xpr->get_rhs()));
                        return true;
                    }
                }
                else if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                {
                    add_constraint(ac_slv.new_assign(rhs_enum_xpr->get_var(), *eq_xpr->get_lhs()));
                    return true;
                }
                else if (auto lhs_atm = dynamic_cast<riddle::atom_term *>(eq_xpr->get_lhs().get())) // we are dealing with an atom constraint..
                {
                    auto rhs_atm = static_cast<riddle::atom_term *>(eq_xpr->get_rhs().get());
                    std::queue<riddle::predicate *> q;
                    q.push(static_cast<riddle::predicate *>(&rhs_atm->get_type()));
                    while (!q.empty())
                    {
                        for (const auto &[f_name, f] : q.front()->get_fields())
                            if (!execute(std::make_shared<riddle::eq_term>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), lhs_atm->get(f_name), rhs_atm->get(f_name))))
                                return false;
                        for (const auto &pp : q.front()->get_parents())
                            q.push(&pp.get());
                        q.pop();
                    }
                    return true;
                }
                else
                    return false;
            }
            else if (auto ge_xpr = dynamic_cast<riddle::ge_term *>(expr.get()))
                return lin_slv.new_gt(static_cast<riddle::arith_item *>(ge_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(ge_xpr->get_rhs().get())->get_lin(), false, c_res ? std::make_optional(std::ref(c_res->get().cnst)) : std::nullopt);
            else if (auto gt_xpr = dynamic_cast<riddle::gt_term *>(expr.get()))
                return lin_slv.new_gt(static_cast<riddle::arith_item *>(gt_xpr->get_lhs().get())->get_lin(), static_cast<riddle::arith_item *>(gt_xpr->get_rhs().get())->get_lin(), true, c_res ? std::make_optional(std::ref(c_res->get().cnst)) : std::nullopt);
            else
                return false; // unsupported expression, just return false..
        }
    }

    void solver_core::add_constraint(arc_consistency::constraint &c) noexcept
    {
        if (c_res)
            c_res->get().ac_cnsts.push_back(c);
        else
            ac_slv.add_constraint(c);
    }

    std::vector<std::reference_wrapper<resolver>> solver_core::get_causes() const noexcept
    {
        if (c_res)
            return {c_res->get()};
        else
            return {};
    }

    void solver_core::compute_resolvers(flaw &flw) noexcept
    {
        c_flaw = flw;
        CURRENT_FLAW(flw);
        assert(!flw.is_expanded());
        flw.compute_resolvers();
        flw.expanded = true;
        switch (flw.resolvers.size())
        {
        case 0:
            break;
        case 1:
            try
            {
                c_res = flw.resolvers[0];
                CURRENT_RESOLVER(flw.resolvers[0].get());
                flw.resolvers[0].get().apply();
                apply_resolver(flw.resolvers[0].get());
                retract_resolver(flw.resolvers[0].get());
            }
            catch (std::exception &)
            { // if applying the resolver fails, we retract it..
                retract_resolver(flw.resolvers[0].get());
                flw.resolvers.clear();
            }
            break;
        default:
            for (auto it = flw.resolvers.begin(); it != flw.resolvers.end();)
                try
                {
                    c_res = *it;
                    CURRENT_RESOLVER(*it);
                    it->get().apply();
                    apply_resolver(it->get());
                    retract_resolver(it->get());
                    ++it;
                }
                catch (std::exception &)
                { // if applying the resolver fails, we retract it and remove it from the list..
                    retract_resolver(it->get());
                    it = flw.resolvers.erase(it);
                }
            break;
        }
        c_res = std::nullopt;
        CURRENT_RESOLVER(std::nullopt);
        c_flaw = std::nullopt;
        CURRENT_FLAW(std::nullopt);
    }

    void solver_core::apply_resolver(resolver &res) noexcept
    {
        lin_slv.add_constraint(res.cnst);
        for (auto &ac_cnst : res.ac_cnsts)
            ac_slv.add_constraint(ac_cnst);
    }

    void solver_core::retract_resolver(resolver &res) noexcept
    {
        lin_slv.retract(res.cnst);
        for (auto &ac_cnst : res.ac_cnsts)
            ac_slv.retract(ac_cnst);
    }

    riddle::atom_state solver_core::get_atom_state(const riddle::atom_term &atm) const noexcept
    {
        switch (ac_slv.sat_val(static_cast<const riddle::atom &>(atm).get_sigma()))
        {
        case utils::True:
            return riddle::active;
        case utils::False:
            return riddle::unified;
        default:
            return riddle::inactive;
        }
    }
} // namespace ratio
