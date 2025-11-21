#include "basic_solver.hpp"
#include "items.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio
{
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
            std::vector<std::reference_wrapper<const utils::enum_val>> ev_refs;
            for (auto &ev_ptr : values)
                ev_refs.emplace_back(*ev_ptr);
            auto ev = ac_slv.new_var(ev_refs);
            // .. and create a new enum flaw to manage the variable..
            auto &ef = new_flaw<enum_flaw>(*this, get_causes(), std::make_shared<riddle::enum_item>(tp, std::move(values), ev));
            open_flaws.insert(&ef);
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
            std::vector<utils::lit> clause;
            clause.reserve(exprs.size());
            for (const riddle::bool_expr &expr : exprs)
                clause.push_back(static_cast<const riddle::bool_item &>(*expr).get_lit());

            add_constraint(ac_slv.new_clause(std::move(clause)));
            auto &cf = new_flaw<clause_flaw>(*this, get_causes(), std::move(exprs));
            open_flaws.insert(&cf);
        }
    }
    void basic_solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        auto &df = new_flaw<disjunction_flaw>(*this, get_causes(), std::move(disjuncts));
        open_flaws.insert(&df);
    }

    void basic_solver::solve()
    {
        if (!ac_slv.propagate() || !lin_slv.check())
            throw std::runtime_error("Unsatisfiable constraints");
    }

    riddle::atom_expr basic_solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args)
    {
        auto &af = new_flaw<atom_flaw>(*this, get_causes(), is_fact, pred, std::move(args), ac_slv.new_sat());
        open_flaws.insert(&af);
        return af.get_atom();
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
