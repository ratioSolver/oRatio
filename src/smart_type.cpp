#include "smart_type.hpp"
#include "graph.hpp"
#include "atom_flaw.hpp"

namespace ratio
{
    smart_type::smart_type(solver &slv, std::string_view name) : riddle::component_type(slv, name), slv(slv) {}

    void smart_type::set_ni(const utils::lit &v) noexcept { slv.get_graph().set_ni(v); }
    void smart_type::restore_ni() noexcept { slv.get_graph().restore_ni(); }

    std::vector<std::reference_wrapper<resolver>> smart_type::get_resolvers(const std::set<atom *> &atms) noexcept
    {
        std::vector<std::reference_wrapper<resolver>> resolvers;
        for (const auto &atm : atms)
            for (const auto &res : atm->get_reason().get_resolvers())
                if (auto *r = dynamic_cast<atom_flaw::activate_fact *>(&res.get()))
                    resolvers.push_back(*r);
                else if (auto *r = dynamic_cast<atom_flaw::activate_goal *>(&res.get()))
                    resolvers.push_back(*r);

        return resolvers;
    }

    atom_listener::atom_listener(riddle::atom &atm) : atm(atm)
    {
        // Add listeners to the atom's solver
        static_cast<solver &>(atm.get_core()).get_lra_theory().add_listener(*this);
        static_cast<solver &>(atm.get_core()).get_rdl_theory().add_listener(*this);
        static_cast<solver &>(atm.get_core()).get_ov_theory().add_listener(*this);
        for (const auto &[name, itm] : atm.get_items())
            if (!is_constant(*itm))
            { // Listen to the variables in the atom
                if (is_bool(*itm))
                    listen_sat(variable(static_cast<riddle::bool_item &>(*itm).get_value()));
                else if (is_int(*itm) || is_real(*itm))
                    for (const auto &v : static_cast<riddle::arith_item &>(*itm).get_value().vars)
                        listen_lra(v.first);
                else if (is_time(*itm))
                    for (const auto &v : static_cast<riddle::arith_item &>(*itm).get_value().vars)
                        listen_rdl(v.first);
                else if (is_enum(*itm))
                    listen_ov(static_cast<riddle::enum_item &>(*itm).get_value());
            }
    }
} // namespace ratio
