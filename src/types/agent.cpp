#include "agent.h"
#include "solver.h"

namespace ratio
{
    agent::agent(riddle::scope &scp) : smart_type(scp, AGENT_NAME) {}

    void agent::new_atom(atom &atm)
    {
        if (atm.is_fact())
        {
            set_ni(atm.get_sigma());
            riddle::expr atm_expr(&atm);
            if (get_solver().get_impulse().is_assignable_from(atm.get_type())) // we apply impulse-predicate whenever the fact becomes active..
                get_solver().get_impulse().call(atm_expr);
            else // we apply interval-predicate whenever the fact becomes active..
                get_solver().get_interval().call(atm_expr);
            restore_ni();
        }

        atoms.emplace_back(&atm);
        // we store, for the atom, its atom listener..
        listeners.emplace_back(new agnt_atom_listener(*this, atm));

        to_check.insert(&atm);
    }

    json::json agent::extract() const noexcept
    {
        json::json tls(json::json_type::array);
        // we partition atoms for each agent they might insist on..
        std::unordered_map<const riddle::item *, std::vector<atom *>> agnt_instances;
        for (auto &agnt_instance : get_instances())
            agnt_instances[&*agnt_instance];
        for (const auto &atm : get_atoms())
            if (get_solver().get_sat_core().value(atm->get_sigma()) == utils::True) // we filter out those which are not strictly active..
            {
                const auto c_scope = atm->get(TAU_KW);
                if (const auto enum_scope = dynamic_cast<enum_item *>(&*c_scope))
                    for (const auto &val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        agnt_instances.at(dynamic_cast<const riddle::item *>(val)).emplace_back(atm);
                else
                    agnt_instances.at(static_cast<riddle::item *>(&*c_scope)).emplace_back(atm);
            }

        for (const auto &[agnt, atms] : agnt_instances)
        {
            json::json tl;
            tl["id"] = get_id(*agnt);
#ifdef COMPUTE_NAMES
            tl["name"] = get_solver().guess_name(*agnt);
#endif
            tl["type"] = AGENT_NAME;

            // for each pulse, the atoms starting at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> starting_atoms;
            // all the pulses of the timeline..
            std::set<utils::inf_rational> pulses;

            for (const auto &atm : atms)
            {
                const auto start = get_solver().arith_value(get_solver().is_impulse(*atm) ? atm->get(RATIO_AT) : atm->get(RATIO_START));
                starting_atoms[start].insert(atm);
                pulses.insert(start);
            }

            json::json j_atms(json::json_type::array);
            for (const auto &p : pulses)
                for (const auto &atm : starting_atoms.at(p))
                    j_atms.push_back(get_id(*atm));
            tl["values"] = std::move(j_atms);

            tls.push_back(std::move(tl));
        }

        return tls;
    }
} // namespace ratio
