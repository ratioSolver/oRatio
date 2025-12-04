#include "basic_flaws.hpp"
#include "items.hpp"

namespace ratio
{
    enum_flaw::enum_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev) noexcept : flaw(slv, std::move(causes)), var(std::make_shared<riddle::enum_item>(*this, tp, std::move(values), ev)) {}

    utils::rational enum_flaw::get_estimated_cost() const noexcept { return get_core().enum_value(*var).size(); }

    clause_flaw::clause_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, std::vector<riddle::bool_expr> &&clause) noexcept : flaw(slv, std::move(causes)), clause(std::move(clause)) {}

    utils::rational clause_flaw::get_estimated_cost() const noexcept { return clause.size(); }

    disjunction_flaw::disjunction_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    utils::rational disjunction_flaw::get_estimated_cost() const noexcept { return disjuncts.size(); }

    atom_flaw::atom_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept : flaw(slv, std::move(causes)), atm(std::make_shared<riddle::atom>(*this, pred, is_fact, std::move(args), std::move(sigma))) {}

    utils::rational atom_flaw::get_estimated_cost() const noexcept
    { // Estimate the cost as the number of active ancestor atoms that can be unified with this atom plus one for activation..
        if (atm->is_fact())
            return utils::rational::zero; // activating a fact has zero cost
        std::size_t count = 0;
        for (auto &a : static_cast<riddle::predicate &>(atm->get_type()).get_atoms())
            if (atm != a && a->get_state() == riddle::active && !have_common_ancestors(a->get_flaw(), atm->get_flaw()) && static_cast<solver &>(get_core()).match(*atm, *a))
                ++count;
        return utils::rational(count + 1);
    }
} // namespace ratio
