#include <cassert>
#include "state_variable.hpp"
#include "solver.hpp"

namespace ratio
{
    state_variable::state_variable(solver &slv) : smart_type(slv, "StateVariable") {}

    void state_variable::new_atom(std::shared_ptr<ratio::atom> &atm) noexcept
    {
        if (atm->is_fact())
        {
            assert(is_interval(*atm));
            set_ni(atm->get_sigma());
            get_solver().get_predicate("Interval")->get().call(atm);
            restore_ni();
        }

        // we store the variables for on-line flaw resolution..
        // the ordering constraints between the atoms are stored in the leqs map..
        const auto start = atm->get("start");
        const auto end = atm->get("end");
        for (const auto &c_atm : atoms)
        {
            const auto c_start = c_atm.get().get("start");
            const auto c_end = c_atm.get().get("end");
#ifdef DL_TN
            auto before = get_solver().get_rdl_theory().new_leq(std::static_pointer_cast<riddle::arith_item>(end)->get_value(), std::static_pointer_cast<riddle::arith_item>(c_start)->get_value());
            auto after = get_solver().get_rdl_theory().new_leq(std::static_pointer_cast<riddle::arith_item>(c_end)->get_value(), std::static_pointer_cast<riddle::arith_item>(start)->get_value());
#elif defined(LRA_TN)
            auto before = get_solver().get_lra_theory().new_leq(std::static_pointer_cast<riddle::arith_item>(end)->get_value(), std::static_pointer_cast<riddle::arith_item>(c_start)->get_value());
            auto after = get_solver().get_lra_theory().new_leq(std::static_pointer_cast<riddle::arith_item>(c_end)->get_value(), std::static_pointer_cast<riddle::arith_item>(start)->get_value());
            [[maybe_unused]] bool nc = get_solver().get_sat().new_clause({!before, !after}); // the ordering constraints are always disjunctive..
            assert(nc);
#endif
            if (get_solver().get_sat().value(before) == utils::Undefined)
                leqs[atm.get()][&c_atm.get()] = before;
            if (get_solver().get_sat().value(after) == utils::Undefined)
                leqs[&c_atm.get()][atm.get()] = after;
        }

        const auto tau = atm->get("tau");
        if (is_enum(*tau))
            for (const auto &sv : get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
            {
                auto var = get_solver().get_ov_theory().allows(static_cast<const riddle::enum_item &>(*tau).get_value(), sv.get());
                if (get_solver().get_sat().value(var) == utils::Undefined)
                    frbs[atm.get()][&sv.get()] = var;
            }

        atoms.push_back(*atm);
        auto listener = std::make_unique<sv_atom_listener>(*this, *atm);
        listener->something_changed();
        listeners.push_back(std::move(listener));
    }

    state_variable::sv_atom_listener::sv_atom_listener(state_variable &sv, atom &a) : atom_listener(a), sv(sv) {}
    void state_variable::sv_atom_listener::something_changed()
    {
        if (sv.get_solver().get_sat().value(atm.get_sigma()) == utils::True)
        { // the atom is active
            const auto tau = atm.get("tau");
            if (is_enum(*tau))
                for (const auto &c_sv : sv.get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
                    sv.to_check.insert(static_cast<const riddle::item *>(&c_sv.get()));
            else
                sv.to_check.insert(tau.get());
        }
    }
} // namespace ratio