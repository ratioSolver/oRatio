#include "atom_flaw.h"
#include "solver.h"
#include <cassert>

namespace ratio
{
    atom_flaw::atom_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, riddle::expr &atm) : flaw(s, causes), atm(atm) { static_cast<atom &>(*atm).reason = this; }

    void atom_flaw::compute_resolvers()
    {
        assert(get_solver().get_sat_core().value(get_phi()) != utils::False);
        // this is the current atom..
        auto &c_atm = static_cast<atom &>(*atm);
        LOG("computing resolvers for " << to_string(c_atm) << "..");
        assert(get_solver().get_sat_core().value(c_atm.sigma) != utils::False);
        // we check if the atom can unify..
        if (get_solver().get_sat_core().value(c_atm.sigma) == utils::Undefined)
            for (auto &i : static_cast<riddle::predicate &>(c_atm.get_type()).get_instances())
            { // we check for possible unifications (i.e. all the instances of the atom's type)..
                if (i.operator->() == &c_atm)
                    continue; // the current atom cannot unify with itself..

                // this is the target (i.e. the atom we are trying to unify with)..
                auto &t_atm = static_cast<atom &>(*i);

                if (!t_atm.reason->is_expanded() ||                                                                   // we cannot unify with an atom that is not expanded..
                    get_solver().get_idl_theory().distance(get_position(), t_atm.reason->get_position()).first > 0 || // we cannot unify with an atom that introduces a cycle..
                    get_solver().get_sat_core().value(t_atm.sigma) == utils::False ||                                 // we cannot unify with an atom that is unified with another atom..
                    get_solver().get_sat_core().value(t_atm.reason->get_phi()) == utils::False ||                     // we cannot unify with an atom that cannot be activated..
                    !get_solver().matches(atm, i))                                                                    // we cannot unify with an atom that does not match the current atom..
                    continue;

                // the equality propositional literal..
                auto eq_lit = static_cast<bool_item &>(*get_solver().eq(atm, i)).get_lit();

                if (get_solver().get_sat_core().value(eq_lit) == utils::False)
                    continue; // the two atoms cannot unify, hence, we skip this instance..

                LOG("found possible unification with " << to_string(t_atm) << "..");

                // we add the resolver..
                auto u_res = new unify_atom(*this, i, eq_lit);
                assert(get_solver().get_sat_core().value(u_res->get_rho()) != utils::False);
                add_resolver(u_res);
            }

        if (c_atm.is_fact())
            if (get_resolvers().empty())
                add_resolver(new activate_fact(*this, get_phi()));
            else
                add_resolver(new activate_fact(*this));
        else if (get_resolvers().empty())
            add_resolver(new activate_goal(*this, get_phi()));
        else
            add_resolver(new activate_goal(*this));
    }

    json::json atom_flaw::get_data() const noexcept { return {{"type", "atom"}, {"atom", {{"id", get_id(*atm)}, {"is_fact", static_cast<atom &>(*atm).is_fact()}, {"type", atm->get_type().get_name()}, {"sigma", variable(static_cast<atom &>(*atm).sigma)}}}}; }

    atom_flaw::activate_fact::activate_fact(atom_flaw &ef) : resolver(ef, utils::rational::ZERO) {}
    atom_flaw::activate_fact::activate_fact(atom_flaw &ef, const semitone::lit &l) : resolver(ef, l, utils::rational::ZERO) {}

    void atom_flaw::activate_fact::apply()
    { // activating this resolver activates the fact..
        auto &af = static_cast<const atom_flaw &>(get_flaw());
        auto &c_atm = static_cast<atom &>(*af.get_atom());
        assert(get_solver().get_sat_core().value(c_atm.sigma) != utils::False);
        assert(get_solver().get_sat_core().value(get_rho()) != utils::False);
        if (!get_solver().get_sat_core().new_clause({!get_rho(), c_atm.sigma}))
            throw riddle::unsolvable_exception();
    }

    json::json atom_flaw::activate_fact::get_data() const noexcept { return {{"type", "activate_fact"}, {"rho", variable(get_rho())}}; }

    atom_flaw::activate_goal::activate_goal(atom_flaw &ef) : resolver(ef, utils::rational::ONE) {}
    atom_flaw::activate_goal::activate_goal(atom_flaw &ef, const semitone::lit &l) : resolver(ef, l, utils::rational::ONE) {}

    void atom_flaw::activate_goal::apply()
    { // activating this resolver activates the goal..
        auto &af = static_cast<atom_flaw &>(get_flaw());
        auto &c_atm = static_cast<atom &>(*af.get_atom());
        assert(get_solver().get_sat_core().value(c_atm.sigma) != utils::False);
        assert(get_solver().get_sat_core().value(get_rho()) != utils::False);
        if (!get_solver().get_sat_core().new_clause({!get_rho(), c_atm.sigma}))
            throw riddle::unsolvable_exception();

        // we apply the corresponding rule..
        static_cast<riddle::predicate &>(c_atm.get_type()).call(af.get_atom());
    }

    json::json atom_flaw::activate_goal::get_data() const noexcept { return {{"type", "activate_goal"}, {"rho", variable(get_rho())}}; }

    atom_flaw::unify_atom::unify_atom(atom_flaw &ef, riddle::expr target, const semitone::lit &unif_lit) : resolver(ef, utils::rational::ZERO), target(target), unif_lit(unif_lit) {}

    void atom_flaw::unify_atom::apply()
    {
        auto &af = static_cast<const atom_flaw &>(get_flaw());
        auto &c_atm = static_cast<atom &>(*af.get_atom());
        auto &t_atm = static_cast<atom &>(*target);
        assert(get_solver().get_sat_core().value(c_atm.sigma) != utils::True);  // the current atom must be unifiable..
        assert(get_solver().get_sat_core().value(t_atm.sigma) != utils::False); // the target atom must be activable..
        assert(get_solver().get_sat_core().value(get_rho()) != utils::False);   // this resolver must be activable..

        // we add a causal link from the target atom's reason to this resolver..
        new_causal_link(t_atm.get_reason());

        assert(t_atm.reason->is_expanded());
        for (auto &r : t_atm.reason->get_resolvers())
            if (dynamic_cast<activate_fact *>(&r.get()) || dynamic_cast<activate_goal *>(&r.get())) // we disable this unification if the target atom is not activable..
                if (!get_solver().get_sat_core().new_clause({!get_rho(), r.get().get_rho()}))
                    throw riddle::unsolvable_exception();

        // as a consequence of the activation of this resolver:
        //  - we make the current atom's sigma false (unified atom)..
        if (!get_solver().get_sat_core().new_clause({!get_rho(), !c_atm.sigma}))
            throw riddle::unsolvable_exception();
        //  - and we make the target atom's sigma true (active atom)..
        if (!get_solver().get_sat_core().new_clause({!get_rho(), t_atm.sigma}))
            throw riddle::unsolvable_exception();
        //  - and we constraint the current atom's and the target atom's variables to be pairwise equal..
        if (!get_solver().get_sat_core().new_clause({!get_rho(), unif_lit}))
            throw riddle::unsolvable_exception();
    }

    json::json atom_flaw::unify_atom::get_data() const noexcept { return {{"type", "unify_atom"}, {"rho", variable(get_rho())}, {"target", get_id(*target)}}; }
} // namespace ratio
