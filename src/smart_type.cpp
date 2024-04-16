#include "smart_type.hpp"
#include "solver.hpp"

namespace ratio
{
    smart_type::smart_type(scope &parent, const std::string &name) : riddle::component_type(parent, name), slv(dynamic_cast<solver &>(parent.get_core())) {}

    atom_listener::~atom_listener()
    {
        // Remove listeners from the atom's solver
        static_cast<solver &>(atm.get_core()).get_lra_theory().remove_listener(*this);
        static_cast<solver &>(atm.get_core()).get_rdl_theory().remove_listener(*this);
        static_cast<solver &>(atm.get_core()).get_ov_theory().remove_listener(*this);
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
