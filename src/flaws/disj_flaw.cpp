#include "disj_flaw.h"

namespace ratio
{
    disj_flaw::disj_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, std::vector<semitone::lit> lits) : flaw(s, std::move(causes)), lits(std::move(lits)) {}

    void disj_flaw::compute_resolvers()
    {
        for (auto &l : lits)
            add_resolver(new choose_lit(*this, utils::rational(1, lits.size()), l));
    }

    json::json disj_flaw::get_data() const noexcept
    {
        json::json j;
        j["type"] = "disj";
        return j;
    }

    disj_flaw::choose_lit::choose_lit(disj_flaw &ef, const utils::rational &cost, const semitone::lit &l) : resolver(ef, l, cost) {}

    json::json disj_flaw::choose_lit::get_data() const noexcept
    {
        json::json j;
        j["lit"] = to_string(get_rho());
        return j;
    }
} // namespace ratio
