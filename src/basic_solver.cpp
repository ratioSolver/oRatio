#include "basic_solver.hpp"
#include "items.hpp"
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

    enum_flaw::enum_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::enum_expr var) noexcept : flaw(slv, std::move(causes)), var(std::move(var)) {}

    void enum_flaw::compute_resolvers() {}

    clause_flaw::clause_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause, const bool &exclusive) noexcept : flaw(slv, std::move(causes), exclusive), clause(std::move(clause)) {}

    void clause_flaw::compute_resolvers() {}

    disjunction_flaw::disjunction_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    void disjunction_flaw::compute_resolvers() {}
} // namespace ratio
