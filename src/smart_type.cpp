#include "smart_type.h"
#include "solver.h"
#include "atom_flaw.h"

namespace ratio
{
    smart_type::smart_type(riddle::scope &scp, const std::string &name) : complex_type(scp, name), slv(static_cast<solver &>(scp.get_core())) {}

    void smart_type::set_ni(const semitone::lit &v) noexcept { slv.set_ni(v); }
    void smart_type::restore_ni() noexcept { slv.restore_ni(); }

    void smart_type::store_flaw(flaw_ptr f) noexcept { slv.new_flaw(std::move(f)); }

    std::vector<std::reference_wrapper<resolver>> smart_type::get_resolvers(const std::set<atom *> &atms) noexcept
    {
        std::unordered_set<resolver *> ress;
        for (const auto &atm : atms)
            for (const auto &r : atm->get_reason().get_resolvers())
                if (resolver *af = dynamic_cast<atom_flaw::activate_fact *>(&r.get()))
                    ress.insert(af);
                else if (resolver *ag = dynamic_cast<atom_flaw::activate_goal *>(&r.get()))
                    ress.insert(ag);

        std::vector<std::reference_wrapper<resolver>> res;
        res.reserve(ress.size());
        for (auto &r : ress)
            res.emplace_back(*r);
        return res;
    }

    atom_listener::atom_listener(atom &atm) : sat_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_sat_core_ptr()), lra_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_lra_theory()), rdl_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_rdl_theory()), ov_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_ov_theory()), atm(atm) {}
} // namespace ratio
