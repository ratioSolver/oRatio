#include "smart_type.h"
#include "solver.h"
#include "field.h"
#include "atom_flaw.h"
#include <cassert>

namespace ratio::solver
{
    smart_type::smart_type(ratio::core::scope &scp, const std::string &name) : ratio::core::type(scp, name, false), slv(static_cast<solver &>(scp.get_core())) {}

    void smart_type::new_atom_flaw(atom_flaw &) {}

    void smart_type::set_ni(const semitone::lit &v) noexcept { get_solver().set_ni(v); }
    void smart_type::restore_ni() noexcept { get_solver().restore_ni(); }

    void smart_type::store_flaw(std::unique_ptr<flaw> f) noexcept { slv.pending_flaws.emplace_back(std::move(f)); }

    std::vector<resolver *> smart_type::get_resolvers(solver &slv, const std::set<ratio::core::atom *> &atms) noexcept
    {
        std::unordered_set<resolver *> ress;
        for (const auto &atm : atms)
            for (const auto &r : slv.atom_properties.at(atm).reason->get_resolvers())
                if (resolver *af = dynamic_cast<atom_flaw::activate_fact *>(r))
                    ress.insert(af);
                else if (resolver *ag = dynamic_cast<atom_flaw::activate_goal *>(r))
                    ress.insert(ag);

        return std::vector<resolver *>(ress.cbegin(), ress.cend());
    }

    atom_listener::atom_listener(ratio::core::atom &atm) : semitone::sat_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_sat_core()), semitone::lra_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_lra_theory()), semitone::rdl_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_rdl_theory()), semitone::ov_value_listener(static_cast<solver &>(atm.get_type().get_core()).get_ov_theory()), atm(atm)
    {
        for (const auto &[xpr_name, xpr] : atm.get_vars())
            if (auto be = dynamic_cast<ratio::core::bool_item *>(&*xpr))
                listen_sat(variable(be->get_value()));
            else if (auto ae = dynamic_cast<ratio::core::arith_item *>(&*xpr))
            {
                if (&ae->get_type() == &atm.get_type().get_core().get_time_type())
                    for (const auto &[v, c] : ae->get_value().vars)
                        listen_rdl(v);
                else
                    for (const auto &[v, c] : ae->get_value().vars)
                        listen_lra(v);
            }
            else if (auto ee = dynamic_cast<ratio::core::enum_item *>(&*xpr))
                listen_set(ee->get_var());
    }
} // namespace ratio::solver