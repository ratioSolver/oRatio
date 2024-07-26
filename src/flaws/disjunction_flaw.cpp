#include <cassert>
#include "disjunction_flaw.hpp"
#include "graph.hpp"

namespace ratio
{
    disjunction_flaw::disjunction_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&conjs) noexcept : flaw(s, std::move(causes)), conjs(std::move(conjs)) { assert(!this->conjs.empty()); }

    void disjunction_flaw::compute_resolvers()
    {
        for (auto &conj : conjs)
        {
            auto cost = conj->compute_cost();
            get_solver().get_graph().new_resolver<choose_conjunction>(*this, *conj, get_solver().arithmetic_value(*cost).get_rational());
        }
    }

    disjunction_flaw::choose_conjunction::choose_conjunction(disjunction_flaw &df, riddle::conjunction &conj, const utils::rational &cost) : resolver(df, cost), conj(conj) {}

    void disjunction_flaw::choose_conjunction::apply() { conj.execute(); }

#ifdef ENABLE_API
    json::json disjunction_flaw::get_data() const noexcept { return {{"type", "disjunction"}}; }

    json::json disjunction_flaw::choose_conjunction::get_data() const noexcept { return {{"type", "choose_conjunction"}}; }
#endif
} // namespace ratio
