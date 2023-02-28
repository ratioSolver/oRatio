#include "smart_type.h"
#include "solver.h"

namespace ratio
{
    smart_type::smart_type(riddle::scope &scp, const std::string &name) : complex_type(scp, name), slv(static_cast<solver &>(scp.get_core())) {}

    void smart_type::set_ni(const semitone::lit &v) noexcept { slv.set_ni(v); }
    void smart_type::restore_ni() noexcept { slv.restore_ni(); }

    void smart_type::store_flaw(flaw_ptr f) noexcept { slv.pending_flaws.emplace_back(std::move(f)); }

    atom_listener::atom_listener(atom &atm) : sat_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_sat_core_ptr()), lra_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_lra_theory()), rdl_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_rdl_theory()), ov_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_ov_theory()), atm(atm) {}
} // namespace ratio
