#include "basic_solver.hpp"
#include "items.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio
{
    flaw::flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes) noexcept : slv(slv), causes(std::move(causes)) {}

    resolver::resolver(flaw &flw, utils::rational &&intrinsic_cost) noexcept : flw(flw), intrinsic_cost(std::move(intrinsic_cost)) {}

    basic_solver::basic_solver() noexcept : solver_core("oRatio Basic Solver") {}

    riddle::expr basic_solver::new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values)
    {
        assert(!values.empty());
        if (values.size() == 1)
        { // Single-valued enum
            assert(&values.front()->get_type() == &tp);
            return values.front();
        }
        else
        {
            std::vector<std::reference_wrapper<resolver>> causes;
            if (c_res)
                causes.push_back(c_res.value());
            std::vector<std::reference_wrapper<const utils::enum_val>> ev_refs;
            for (auto &ev_ptr : values)
                ev_refs.emplace_back(*ev_ptr);
            auto ev = ac_slv.new_var(ev_refs);
            // .. and create a new enum flaw to manage the variable..
            auto &ef = new_flaw<enum_flaw>(*this, std::move(causes), std::make_shared<riddle::enum_item>(tp, std::move(values), ev));
            return ef.get_var();
        }
    }

    void basic_solver::new_clause(std::vector<riddle::bool_expr> &&exprs)
    {
        assert(!exprs.empty());
        if (exprs.size() == 1)
        { // if there is only one expression, just execute it..
            if (!execute(exprs[0]))
                throw std::runtime_error("Unsatisfiable constraints");
        }
        else
        { // otherwise, create a new clause flaw..
            std::vector<std::reference_wrapper<resolver>> causes;
            if (c_res)
                causes.push_back(c_res.value());
            std::vector<utils::lit> clause;
            clause.reserve(exprs.size());
            for (const riddle::bool_expr &expr : exprs)
                clause.push_back(static_cast<const riddle::bool_item &>(*expr).get_lit());

            auto &ac_cnstr = ac_slv.new_clause(std::move(clause));
            if (c_res) // if there is a current resolver, add the expression to it..
                c_res->get().ac_cnsts.push_back(ac_cnstr);
            else
                ac_slv.add_constraint(ac_cnstr);
            new_flaw<clause_flaw>(*this, std::move(causes), std::move(exprs));
        }
    }
    void basic_solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<std::reference_wrapper<resolver>> causes;
        if (c_res)
            causes.push_back(c_res.value());

        new_flaw<disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
    }

    void basic_solver::solve() {}

    riddle::atom_expr basic_solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args)
    {
        std::vector<std::reference_wrapper<resolver>> causes;
        if (c_res)
            causes.push_back(c_res.value());

        auto &af = new_flaw<atom_flaw>(*this, std::move(causes), is_fact, pred, std::move(args), ac_slv.new_sat());
        return af.get_atom();
    }

    bool basic_solver::execute(const riddle::bool_expr &expr) noexcept
    {
        if (auto n_xpr = dynamic_cast<riddle::bool_not *>(expr.get()))
        {
            if (auto b_xpr = dynamic_cast<riddle::bool_item *>(n_xpr->get_arg().get()))
            {
                auto &a_cnstr = ac_slv.new_assign(utils::variable(b_xpr->get_lit()), utils::sign(b_xpr->get_lit()) ? arc_consistency::solver::False : arc_consistency::solver::True);
                ac_slv.add_constraint(a_cnstr);
                if (c_res)
                    c_res->get().ac_cnsts.push_back(a_cnstr);
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
                    auto &neq_cnstr = ac_slv.new_distinct(utils::variable(lhs_bxpr->get_lit()), utils::variable(static_cast<riddle::bool_item &>(*eq_xpr->get_rhs()).get_lit()));
                    ac_slv.add_constraint(neq_cnstr);
                    if (c_res) // if there is a current resolver, add the expression to it..
                        c_res->get().ac_cnsts.push_back(neq_cnstr);
                    return true;
                }
                else if (auto lhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_lhs().get())) // we are dealing with an enum constraint..
                {
                    if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                    { // both sides are enum items..
                        auto &neq_cnstr = ac_slv.new_distinct(lhs_enum_xpr->get_var(), rhs_enum_xpr->get_var());
                        ac_slv.add_constraint(neq_cnstr);
                        if (c_res) // if there is a current resolver, add the expression to it..
                            c_res->get().ac_cnsts.push_back(neq_cnstr);
                        return true;
                    }
                    else
                    {
                        auto &neq_cnstr = ac_slv.new_forbid(lhs_enum_xpr->get_var(), *eq_xpr->get_rhs());
                        ac_slv.add_constraint(neq_cnstr);
                        if (c_res) // if there is a current resolver, add the expression to it..
                            c_res->get().ac_cnsts.push_back(neq_cnstr);
                        return true;
                    }
                }
                else if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                {
                    auto &neq_cnstr = ac_slv.new_forbid(rhs_enum_xpr->get_var(), *eq_xpr->get_lhs());
                    ac_slv.add_constraint(neq_cnstr);
                    if (c_res) // if there is a current resolver, add the expression to it..
                        c_res->get().ac_cnsts.push_back(neq_cnstr);
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
                auto &a_cnstr = ac_slv.new_assign(utils::variable(b_xpr->get_lit()), utils::sign(b_xpr->get_lit()) ? arc_consistency::solver::True : arc_consistency::solver::False);
                ac_slv.add_constraint(a_cnstr);
                if (c_res)
                    c_res->get().ac_cnsts.push_back(a_cnstr);
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
                    auto &eq_cnstr = ac_slv.new_equal(utils::variable(lhs_bxpr->get_lit()), utils::variable(static_cast<riddle::bool_item &>(*eq_xpr->get_rhs()).get_lit()));
                    ac_slv.add_constraint(eq_cnstr);
                    if (c_res) // if there is a current resolver, add the expression to it..
                        c_res->get().ac_cnsts.push_back(eq_cnstr);
                    return true;
                }
                else if (auto lhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_lhs().get())) // we are dealing with an enum constraint..
                {
                    if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                    { // both sides are enum items..
                        auto &eq_cnstr = ac_slv.new_equal(lhs_enum_xpr->get_var(), rhs_enum_xpr->get_var());
                        ac_slv.add_constraint(eq_cnstr);
                        if (c_res) // if there is a current resolver, add the expression to it..
                            c_res->get().ac_cnsts.push_back(eq_cnstr);
                        return true;
                    }
                    else
                    {
                        auto &eq_cnstr = ac_slv.new_assign(lhs_enum_xpr->get_var(), *eq_xpr->get_rhs());
                        ac_slv.add_constraint(eq_cnstr);
                        if (c_res) // if there is a current resolver, add the expression to it..
                            c_res->get().ac_cnsts.push_back(eq_cnstr);
                        return true;
                    }
                }
                else if (auto rhs_enum_xpr = dynamic_cast<riddle::enum_item *>(eq_xpr->get_rhs().get()))
                {
                    auto &eq_cnstr = ac_slv.new_assign(rhs_enum_xpr->get_var(), *eq_xpr->get_lhs());
                    ac_slv.add_constraint(eq_cnstr);
                    if (c_res) // if there is a current resolver, add the expression to it..
                        c_res->get().ac_cnsts.push_back(eq_cnstr);
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

    enum_flaw::enum_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::enum_expr var) noexcept : flaw(slv, std::move(causes)), var(std::move(var)) {}

    void enum_flaw::compute_resolvers() {}

    clause_flaw::clause_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause) noexcept : flaw(slv, std::move(causes)), clause(std::move(clause)) {}

    void clause_flaw::compute_resolvers() {}

    disjunction_flaw::disjunction_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    void disjunction_flaw::compute_resolvers() {}

    atom_flaw::atom_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept : flaw(slv, std::move(causes)), atm(std::make_shared<riddle::atom>(pred, is_fact, std::move(args), std::move(sigma))) {}

    void atom_flaw::compute_resolvers() {}
} // namespace ratio
