#include "disjunction_flaw.h"
#include "solver.h"

namespace ratio::solver
{
    disjunction_flaw::disjunction_flaw(solver &slv, std::vector<resolver *> causes, std::vector<std::unique_ptr<ratio::core::conjunction>> conjs) : flaw(slv, std::move(causes), false), conjs(std::move(conjs)) {}

    void disjunction_flaw::compute_resolvers()
    {
        for (auto &cnj : conjs)
            add_resolver(std::make_unique<choose_conjunction>(*this, std::move(cnj)));
    }

    disjunction_flaw::choose_conjunction::choose_conjunction(disjunction_flaw &disj_flaw, std::unique_ptr<ratio::core::conjunction> conj) : resolver(conj->get_cost(), disj_flaw), conj(std::move(conj)) {}

    void disjunction_flaw::choose_conjunction::apply() { conj->execute(); }
} // namespace ratio::solver