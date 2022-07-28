#include "var_flaw.h"
#include "solver.h"

namespace ratio::solver
{
    var_flaw::var_flaw(solver &slv, std::vector<resolver *> causes, ratio::core::enum_item &v_itm) : flaw(slv, std::move(causes), true), v_itm(v_itm) {}

    void var_flaw::compute_resolvers()
    {
        std::unordered_set<semitone::var_value *> vals = get_solver().get_ov_theory().value(v_itm.get_var());
        for (const auto &v : vals)
            add_resolver(std::make_unique<choose_value>(semitone::rational(1, static_cast<semitone::I>(vals.size())), *this, *v));
    }

    var_flaw::choose_value::choose_value(semitone::rational cst, var_flaw &enm_flaw, semitone::var_value &val) : resolver(enm_flaw.get_solver().get_ov_theory().allows(enm_flaw.v_itm.get_var(), val), cst, enm_flaw), v(enm_flaw.v_itm.get_var()), val(val) {}

    void var_flaw::choose_value::apply()
    { // activating this resolver assigns a value to the variable..
        if (!get_solver().get_sat_core().new_clause({!get_rho(), get_solver().get_ov_theory().allows(static_cast<var_flaw &>(get_effect()).v_itm.get_var(), val)}))
            throw ratio::core::unsolvable_exception();
    }
} // namespace ratio::solver