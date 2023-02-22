#include "enum_flaw.h"
#include "solver.h"
#include "ov_theory.h"

namespace ratio
{
    enum_flaw::enum_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, enum_item &ei) : flaw(s, std::move(causes), true), ei(ei) {}

    void enum_flaw::compute_resolvers()
    { // we add a resolver for each possible value of the enum..
        auto dom = get_solver().domain(&ei);
        for (auto &v : dom)
            add_resolver(new enum_resolver(*this, utils::rational(1, dom.size()), static_cast<riddle::complex_item &>(*v)));
    }

    json::json enum_flaw::get_data() const noexcept
    {
        json::json j;
        j["type"] = "enum";
        return j;
    }

    enum_flaw::enum_resolver::enum_resolver(enum_flaw &ef, const utils::rational &cost, utils::enum_val &val) : resolver(ef, cost), val(val) {}

    void enum_flaw::enum_resolver::apply()
    { // we add a clause to the SAT solver that enforces the enum value as a consequence of the resolver's activation..
        if (!get_solver().get_sat_core().new_clause({!get_rho(), get_solver().get_ov_theory().allows(static_cast<const enum_flaw &>(get_flaw()).ei.get_var(), val)}))
            throw riddle::unsolvable_exception();
    }

    json::json enum_flaw::enum_resolver::get_data() const noexcept
    {
        json::json j;
        j["type"] = "assignment";
#ifdef COMPUTE_NAMES
        auto name = get_solver().guess_name(static_cast<complex_item &>(val));
        if (!name.empty())
            j["name"] = name;
#endif
        j["value"] = value(static_cast<riddle::complex_item &>(val));
        return j;
    }
} // namespace ratio
