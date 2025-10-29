#include "graph.hpp"
#include "solver.hpp"

namespace ratio
{
    flaw::flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, const bool &exclusive) noexcept : slv(slv), causes(std::move(causes)), exclusive(exclusive) {}

    enum_flaw::enum_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> var) noexcept : flaw(slv, std::move(causes)), var(std::move(var)) {}

    clause_flaw::clause_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause, std::shared_ptr<arc_consistency::constraint> ac_cnstr, const bool &exclusive) noexcept : flaw(slv, std::move(causes), exclusive), clause(std::move(clause)), ac_cnstr(std::move(ac_cnstr)) {}

    disjunction_flaw::disjunction_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    atom_flaw::atom_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept : flaw(slv, std::move(causes)), atm(std::make_shared<atom>(*this, pred, is_fact, std::move(args), std::move(sigma))) {}
} // namespace ratio
