#include "disjunction_flaw.h"
#include "solver.h"

namespace ratio
{
    disjunction_flaw::disjunction_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, std::vector<riddle::conjunction_ptr> xprs) : flaw(s, std::move(causes), true), xprs(std::move(xprs)) {}

    void disjunction_flaw::compute_resolvers()
    {
        for (const auto &xpr : xprs)
            add_resolver(new choose_conjunction(*this, xpr));
    }

    json::json disjunction_flaw::get_data() const noexcept { return {{"type", "disjunction"}}; }

    disjunction_flaw::choose_conjunction::choose_conjunction(disjunction_flaw &df, const riddle::conjunction_ptr &xpr) : resolver(df, xpr->get_cost()), xpr(xpr) {}

    void disjunction_flaw::choose_conjunction::apply() { xpr->execute(); }

    json::json disjunction_flaw::choose_conjunction::get_data() const noexcept { return {{"type", "choose_conjunction"}, {"cost", to_json(xpr->get_cost())}}; }
} // namespace ratio
