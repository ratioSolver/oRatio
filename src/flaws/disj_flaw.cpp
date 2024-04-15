#include <cassert>
#include "disj_flaw.hpp"
#include "solver.hpp"
#include "graph.hpp"

namespace ratio
{
    disj_flaw::disj_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<utils::lit> &&lits, bool exclusive) noexcept : flaw(s, std::move(causes), exclusive), lits(lits) { assert(!lits.empty()); }

    void disj_flaw::compute_resolvers()
    {
        for (const auto &l : lits)
            if (get_solver().get_sat().value(l) != utils::False)
                get_solver().get_graph().new_resolver<choose_lit>(*this, utils::rational::one / lits.size(), l);
    }

    disj_flaw::choose_lit::choose_lit(disj_flaw &ef, const utils::rational &cost, const utils::lit &l) : resolver(ef, l, cost) {}

#ifdef ENABLE_VISUALIZATION
    json::json disj_flaw::get_data() const noexcept { return {{"type", "disj"}}; }

    json::json disj_flaw::choose_lit::get_data() const noexcept { return {{"type", "choose_lit"}}; }
#endif
} // namespace ratio
