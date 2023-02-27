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
                    !get_solver().matches(atm, i))                                                                    // we cannot unify with an atom that does not match the current atom..
                    continue;

                // the equality propositional literal..
                auto eq_lit = static_cast<bool_item &>(*get_solver().eq(atm, i)).get_lit();

                if (get_solver().get_sat_core().value(eq_lit) == utils::False)
                    continue; // the two atoms cannot unify, hence, we skip this instance..

                // we add the resolver..
                auto u_res = new unify_atom(*this, i, eq_lit);
                assert(get_solver().get_sat_core().value(u_res->get_rho()) != utils::False);
                add_resolver(u_res);
                get_solver().new_causal_link(*this, *u_res);
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

    json::json atom_flaw::get_data() const noexcept
    {
        json::json data;
        data["type"] = "atom_flaw";
        json::json atm_data;
        atm_data["id"] = get_id(*atm);
        atm_data["type"] = atm->get_type().get_name();
        atm_data["sigma"] = variable(static_cast<atom &>(*atm).sigma);
        data["atom"] = atm_data;
        return data;
    }

    atom_flaw::activate_fact::activate_fact(atom_flaw &ef) : resolver(ef, utils::rational::ZERO) {}
    atom_flaw::activate_fact::activate_fact(atom_flaw &ef, const semitone::lit &l) : resolver(ef, l, utils::rational::ZERO) {}

    void atom_flaw::activate_fact::apply()
    {
        auto &af = static_cast<const atom_flaw &>(get_flaw());
        auto &c_atm = static_cast<atom &>(*af.get_atom());
        assert(get_solver().get_sat_core().value(c_atm.sigma) != utils::False);
        assert(get_solver().get_sat_core().value(get_rho()) != utils::False);
        if (!get_solver().get_sat_core().new_clause({get_rho(), c_atm.sigma}))
            throw riddle::unsolvable_exception();
    }

    json::json atom_flaw::activate_fact::get_data() const noexcept
    {
        json::json data;
        data["type"] = "activate_fact";
        data["rho"] = variable(get_rho());
        return data;
    }

    atom_flaw::activate_goal::activate_goal(atom_flaw &ef) : resolver(ef, utils::rational::ZERO) {}
    atom_flaw::activate_goal::activate_goal(atom_flaw &ef, const semitone::lit &l) : resolver(ef, l, utils::rational::ZERO) {}

    void atom_flaw::activate_goal::apply()
    {
        auto &af = static_cast<atom_flaw &>(get_flaw());
        auto &c_atm = static_cast<atom &>(*af.get_atom());
        assert(get_solver().get_sat_core().value(c_atm.sigma) != utils::False);
        assert(get_solver().get_sat_core().value(get_rho()) != utils::False);
        if (!get_solver().get_sat_core().new_clause({get_rho(), c_atm.sigma}))
            throw riddle::unsolvable_exception();

        // we apply the corresponding rule..
        static_cast<riddle::predicate &>(c_atm.get_type()).call(af.get_atom());
    }

    json::json atom_flaw::activate_goal::get_data() const noexcept
    {
        json::json data;
        data["type"] = "activate_goal";
        data["rho"] = variable(get_rho());
        return data;
    }

    atom_flaw::unify_atom::unify_atom(atom_flaw &ef, riddle::expr target, const semitone::lit &unif_lit) : resolver(ef, unif_lit, utils::rational::ZERO), target(target) {}

    void atom_flaw::unify_atom::apply()
    {
        auto &af = static_cast<const atom_flaw &>(get_flaw());
        auto &c_atm = static_cast<atom &>(*af.get_atom());
        auto &t_atm = static_cast<atom &>(*target);
        assert(get_solver().get_sat_core().value(c_atm.sigma) != utils::False);
        assert(get_solver().get_sat_core().value(t_atm.sigma) != utils::False);
        assert(get_solver().get_sat_core().value(get_rho()) != utils::False);
        if (!get_solver().get_sat_core().new_clause({get_rho(), c_atm.sigma, t_atm.sigma}))
            throw riddle::unsolvable_exception();

        assert(t_atm.reason->is_expanded());
        for (auto &r : t_atm.reason->get_resolvers())
            if (dynamic_cast<activate_fact *>(&r.get()) || dynamic_cast<activate_goal *>(&r.get()))
            { // we disable this unification if the target atom is not activable..
                if (get_solver().get_sat_core().new_clause({!get_rho(), r.get().get_rho()}))
                    throw riddle::unsolvable_exception();
            }

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

    json::json atom_flaw::unify_atom::get_data() const noexcept
    {
        json::json data;
        data["type"] = "unify_atom";
        data["rho"] = variable(get_rho());
        data["target"] = get_id(*target);
        return data;
    }
} // namespace ratio