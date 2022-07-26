#include "atom_flaw.h"
#include "solver.h"
#include "predicate.h"
#include <cassert>

namespace ratio::solver
{
    atom_flaw::atom_flaw(solver &slv, std::vector<resolver *> causes, ratio::core::expr &atm, const bool is_fact) : flaw(slv, std::move(causes), true), atm(atm), is_fact(is_fact) {}

    ORATIOSOLVER_EXPORT json::json atom_flaw::get_data() const noexcept
    {
        json::json j_f;
        j_f["type"] = is_fact ? "fact" : "goal";
        json::json j_atm;
        j_atm["id"] = get_id(get_atom());
        j_atm["sigma"] = get_sigma(get_solver(), get_atom());
        j_atm["type"] = get_atom().get_type().get_name();
        j_f["atom"] = std::move(j_atm);
        return j_f;
    }

    void atom_flaw::compute_resolvers()
    {
        assert(get_solver().get_sat_core()->value(get_phi()) != semitone::False);
        assert(get_solver().get_sat_core()->value(get_sigma(get_solver(), get_atom())) != semitone::False);
        if (get_solver().get_sat_core()->value(get_sigma(get_solver(), get_atom())) == semitone::Undefined) // we check if the atom can unify..
            for (const auto &i : atm->get_type().get_instances())
            { // we check for possible unifications (i.e. all the instances of the atom's type)..
                if (&*i == &get_atom())
                    continue; // the current atom cannot unify with itself..

                // this is the target (i.e. the atom we are trying to unify with)..
                ratio::core::atom &t_atm = static_cast<ratio::core::atom &>(*i);

                // this is the target flaw (i.e. the one we are trying to unify with) and cannot be in the current flaw's causes' effects..
                atom_flaw &t_flaw = get_reason(get_solver(), t_atm);

                if (!t_flaw.is_expanded() ||                                                                   // the target flaw must hav been already expanded..
                    get_solver().get_idl_theory().distance(get_position(), t_flaw.get_position()).first > 0 || // unifying with the target atom would introduce cyclic causality..
                    get_solver().get_sat_core()->value(get_sigma(get_solver(), t_atm)) == semitone::False ||   // the target atom is unified with some other atom..
                    !get_solver().matches(get_atom(), t_atm))                                                  // the atom does not equate with the target target..
                    continue;

                // the equality propositional literal..
                semitone::lit eq_lit = get_solver().eq(get_atom(), t_atm);

                if (get_solver().get_sat_core()->value(eq_lit) == semitone::False)
                    continue; // the two atoms cannot unify, hence, we skip this instance..

                auto u_res = std::make_unique<unify_atom>(*this, t_atm, std::vector<semitone::lit>({semitone::lit(get_sigma(get_solver(), get_atom()), false), semitone::lit(get_solver().atom_properties.at(&t_atm).sigma), eq_lit}));
                assert(get_solver().get_sat_core()->value(u_res->get_rho()) != semitone::False);
                auto &cl_res = *u_res;
                add_resolver(std::move(u_res));
                get_solver().new_causal_link(t_flaw, cl_res);
            }

        if (is_fact)
            if (get_resolvers().empty())
                add_resolver(std::make_unique<activate_fact>(get_phi(), *this));
            else
                add_resolver(std::make_unique<activate_fact>(*this));
        else if (get_resolvers().empty())
            add_resolver(std::make_unique<activate_goal>(get_phi(), *this));
        else
            add_resolver(std::make_unique<activate_goal>(*this));
    }

    atom_flaw::activate_fact::activate_fact(atom_flaw &f) : resolver(semitone::rational::ZERO, f) {}
    atom_flaw::activate_fact::activate_fact(const semitone::lit &r, atom_flaw &f) : resolver(r, semitone::rational::ZERO, f) {}

    ORATIOSOLVER_EXPORT json::json atom_flaw::activate_fact::get_data() const noexcept
    {
        json::json j_r;
        j_r["type"] = "activate";
        return j_r;
    }

    void atom_flaw::activate_fact::apply()
    { // activating this resolver activates the fact..
        if (!get_solver().get_sat_core()->new_clause({!get_rho(), semitone::lit(get_sigma(get_solver(), static_cast<atom_flaw &>(get_effect()).get_atom()))}))
            throw ratio::core::unsolvable_exception();
    }

    atom_flaw::activate_goal::activate_goal(atom_flaw &f) : resolver(semitone::rational::ONE, f) {}
    atom_flaw::activate_goal::activate_goal(const semitone::lit &r, atom_flaw &f) : resolver(r, semitone::rational::ONE, f) {}

    ORATIOSOLVER_EXPORT json::json atom_flaw::activate_goal::get_data() const noexcept
    {
        json::json j_r;
        j_r["type"] = "activate";
        return j_r;
    }

    void atom_flaw::activate_goal::apply()
    { // activating this resolver activates the goal..
        if (!get_solver().get_sat_core()->new_clause({!get_rho(), semitone::lit(get_sigma(get_solver(), static_cast<atom_flaw &>(get_effect()).get_atom()))}))
            throw ratio::core::unsolvable_exception();
        // we also apply the rule..
        static_cast<ratio::core::predicate &>(static_cast<atom_flaw &>(get_effect()).get_atom().get_type()).apply_rule(static_cast<atom_flaw &>(get_effect()).atm);
    }

    atom_flaw::unify_atom::unify_atom(atom_flaw &f, ratio::core::atom &trgt, const std::vector<semitone::lit> &unif_lits) : resolver(semitone::rational::ONE, f), trgt(trgt), unif_lits(unif_lits) {}

    ORATIOSOLVER_EXPORT json::json atom_flaw::unify_atom::get_data() const noexcept
    {
        json::json j_r;
        j_r["type"] = "unify";
        j_r["target"] = get_id(trgt);
        return j_r;
    }

    void atom_flaw::unify_atom::apply()
    {
        // this is the target flaw..
        atom_flaw &t_flaw = get_reason(get_solver(), trgt);
        assert(t_flaw.is_expanded());
        for (const auto &r : t_flaw.get_resolvers())
            if (activate_fact *act_f = dynamic_cast<activate_fact *>(r))
            { // we disable this unification if the target fact is not activable..
                if (!get_solver().get_sat_core()->new_clause({act_f->get_rho(), !get_rho()}))
                    throw ratio::core::unsolvable_exception();
            }
            else if (activate_goal *act_g = dynamic_cast<activate_goal *>(r))
            { // we disable this unification if the target goal is not activable..
                if (!get_solver().get_sat_core()->new_clause({act_g->get_rho(), !get_rho()}))
                    throw ratio::core::unsolvable_exception();
            }

        // activating this resolver entails the unification..
        for (const auto &v : unif_lits)
            if (!get_solver().get_sat_core()->new_clause({!get_rho(), v}))
                throw ratio::core::unsolvable_exception();
    }
} // namespace ratio::solver