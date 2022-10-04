#include "disj_flaw.h"
#include "solver.h"

namespace ratio::solver
{
    disj_flaw::disj_flaw(solver &slv, std::vector<resolver *> causes, std::vector<semitone::lit> lits) : flaw(slv, std::move(causes), false), lits(std::move(lits)) {}

    ORATIOSOLVER_EXPORT json::json disj_flaw::get_data() const noexcept
    {
        json::json j_f;
        j_f["type"] = "disj";
        return j_f;
    }

    void disj_flaw::compute_resolvers()
    {
        for (const auto &p : lits)
            add_resolver(std::make_unique<choose_lit>(semitone::rational(1, static_cast<semitone::I>(lits.size())), *this, p));
    }

    disj_flaw::choose_lit::choose_lit(semitone::rational cst, disj_flaw &disj_flaw, const semitone::lit &p) : resolver(p, cst, disj_flaw) {}

    ORATIOSOLVER_EXPORT json::json disj_flaw::choose_lit::get_data() const noexcept { return json::object(); }

    void disj_flaw::choose_lit::apply() {}
} // namespace ratio::solver