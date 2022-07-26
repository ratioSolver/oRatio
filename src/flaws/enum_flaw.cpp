#include "enum_flaw.h"
#include "solver.h"

namespace ratio::solver
{
    enum_flaw::enum_flaw(solver &slv, std::vector<resolver *> causes, ratio::core::enum_item &v_itm) : flaw(slv, std::move(causes), true), v_itm(v_itm) {}

    json::json enum_flaw::get_data() const noexcept
    {
        json::json j_f;
        j_f["type"] = "enum";
        return j_f;
    }

    void enum_flaw::compute_resolvers()
    {
        std::unordered_set<semitone::var_value *> vals = get_solver().get_ov_theory().value(v_itm.get_var());
        for (const auto &v : vals)
            add_resolver(std::make_unique<choose_value>(semitone::rational(1, static_cast<semitone::I>(vals.size())), *this, *v));
    }

    enum_flaw::choose_value::choose_value(semitone::rational cst, enum_flaw &enm_flaw, semitone::var_value &val) : resolver(enm_flaw.get_solver().get_ov_theory().allows(enm_flaw.v_itm.get_var(), val), cst, enm_flaw), v(enm_flaw.v_itm.get_var()), val(val) {}

    json::json enum_flaw::choose_value::get_data() const noexcept
    {
        json::json j_r;
        j_r["type"] = "assignment";
#ifdef COMPUTE_NAMES
        auto name = get_solver().guess_name(static_cast<ratio::core::item &>(val));
        if (!name.empty())
            j_r["name"] = name;
#endif
        j_r["value"] = value(static_cast<ratio::core::item &>(val));
        return j_r;
    }

    void enum_flaw::choose_value::apply()
    { // activating this resolver assigns a value to the variable..
        if (!get_solver().get_sat_core()->new_clause({!get_rho(), get_solver().get_ov_theory().allows(static_cast<enum_flaw &>(get_effect()).v_itm.get_var(), val)}))
            throw ratio::core::unsolvable_exception();
    }
} // namespace ratio::solver