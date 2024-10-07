#include <cassert>
#include "agent.hpp"
#include "solver.hpp"

namespace ratio
{
    agent::agent(solver &slv) : smart_type(slv, AGENT_TYPE_NAME) { add_constructor(std::make_unique<riddle::constructor>(*this, std::vector<std::unique_ptr<riddle::field>>{}, std::vector<riddle::init_element>{}, std::vector<std::unique_ptr<riddle::statement>>{})); }

    void agent::new_atom(std::shared_ptr<ratio::atom> &atm) noexcept
    {
        if (atm->is_fact())
        {
            set_ni(atm->get_sigma());
            if (is_impulse(*atm))
                get_solver().get_predicate(IMPULSE_PREDICATE_NAME)->get().call(atm);
            else
                get_solver().get_predicate(INTERVAL_PREDICATE_NAME)->get().call(atm);
            restore_ni();
        }
        atoms.push_back(*atm);
    }

#ifdef ENABLE_API
    json::json agent::extract() const noexcept
    {
        json::json tls(json::json_type::array);
        // we partition atoms for each agent they might insist on..
        std::unordered_map<const riddle::component *, std::vector<atom *>> agnt_instances;
        for (const auto &agnt : get_instances())
            agnt_instances.emplace(static_cast<riddle::component *>(agnt.get()), std::vector<atom *>());
        for (const auto &atm : atoms)
            if (get_solver().get_sat().value(atm.get().get_sigma()) == utils::True)
            { // the atom is active..
                const auto tau = atm.get().get(riddle::TAU_NAME);
                if (is_enum(*tau)) // the `tau` parameter is a variable..
                    for (const auto &c_agnt : get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
                        agnt_instances.at(static_cast<const riddle::component *>(&c_agnt.get())).push_back(&atm.get());
                else // the `tau` parameter is a constant..
                    agnt_instances.at(static_cast<const riddle::component *>(tau.get())).push_back(&atm.get());
            }

        for (const auto &[agnt, atms] : agnt_instances)
        {
            json::json tl{{"id", get_id(*agnt)}, {"type", AGENT_TYPE_NAME}, {"name", get_solver().guess_name(*agnt)}};

            // for each pulse, the atoms starting at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> starting_atoms;
            // all the pulses of the timeline..
            std::set<utils::inf_rational> pulses;

            for (const auto &atm : atms)
            {
                const auto start = get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(is_impulse(*atm) ? atm->get(AT_NAME) : atm->get(START_NAME)));
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
#endif
} // namespace ratio