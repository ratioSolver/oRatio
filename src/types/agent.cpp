#include "agent.h"
#include "solver.h"
#include "predicate.h"
#include "field.h"
#include "atom_flaw.h"
#include <assert.h>

namespace ratio::solver
{
    agent::agent(solver &slv) : smart_type(slv, AGENT_NAME) { new_constructor(std::make_unique<agnt_constructor>(*this)); }

    std::vector<std::vector<std::pair<semitone::lit, double>>> agent::get_current_incs()
    {
        std::vector<std::vector<std::pair<semitone::lit, double>>> incs;
        // TODO: add code for finding inconsistencies here..
        return incs;
    }

    void agent::new_predicate(ratio::core::predicate &pred) noexcept
    {
        assert(get_solver().is_impulse(pred) || get_solver().is_interval(pred));
        // each agent predicate has a tau parameter indicating on which agents the atoms insist on..
        new_field(pred, std::make_unique<ratio::core::field>(static_cast<ratio::core::type &>(pred.get_scope()), TAU_KW));
    }

    void agent::new_atom_flaw(atom_flaw &f)
    {
        auto &atm = f.get_atom();
        if (f.is_fact)
        {
            set_ni(semitone::lit(get_sigma(get_solver(), atm)));
            auto atm_expr = f.get_atom_expr();
            if (get_solver().get_impulse().is_assignable_from(atm.get_type())) // we apply impulse-predicate whenever the fact becomes active..
                get_solver().get_impulse().apply_rule(atm_expr);
            else // we apply interval-predicate whenever the fact becomes active..
                get_solver().get_interval().apply_rule(atm_expr);
            restore_ni();
        }

        // we store, for the atom, its atom listener..
        atoms.emplace_back(&atm);
        listeners.emplace_back(std::make_unique<agnt_atom_listener>(*this, atm));

        to_check.insert(&atm);
    }

    agent::agnt_constructor::agnt_constructor(agent &agnt) : ratio::core::constructor(agnt, {}, {}, {}, {}) {}
    agent::agnt_constructor::~agnt_constructor() {}

    agent::agnt_atom_listener::agnt_atom_listener(agent &agnt, ratio::core::atom &atm) : atom_listener(atm), agnt(agnt) {}
    agent::agnt_atom_listener::~agnt_atom_listener() {}
    void agent::agnt_atom_listener::something_changed() { agnt.to_check.insert(&atm); }

    json::json agent::extract() const noexcept
    {
        json::array tls;
        // we partition atoms for each agent they might insist on..
        std::unordered_map<const ratio::core::item *, std::vector<ratio::core::atom *>> agnt_instances;
        for (auto &agnt_instance : get_instances())
            agnt_instances[&*agnt_instance];
        for (const auto &atm : get_atoms())
            if (get_solver().get_sat_core()->value(get_sigma(get_solver(), *atm)) == semitone::True) // we filter out those which are not strictly active..
            {
                const auto c_scope = atm->get(TAU_KW);
                if (const auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope))
                    for (const auto &val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        agnt_instances.at(static_cast<const ratio::core::item *>(val)).emplace_back(atm);
                else
                    agnt_instances.at(static_cast<ratio::core::item *>(&*c_scope)).emplace_back(atm);
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
            std::map<semitone::inf_rational, std::set<ratio::core::atom *>> starting_atoms;
            // all the pulses of the timeline..
            std::set<semitone::inf_rational> pulses;

            for (const auto &atm : atms)
            {
                const auto start = get_solver().ratio::core::core::arith_value(get_solver().is_impulse(*atm) ? atm->get(RATIO_AT) : atm->get(RATIO_START));
                starting_atoms[start].insert(atm);
                pulses.insert(start);
            }

            json::array j_atms;
            for (const auto &p : pulses)
                for (const auto &atm : starting_atoms.at(p))
                    j_atms.push_back(get_id(*atm));
            tl["values"] = std::move(j_atms);

            tls.push_back(std::move(tl));
        }

        return tls;
    }
} // namespace ratio::solver