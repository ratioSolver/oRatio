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
} // namespace ratio::solver