#include <cassert>
#include "atom_flaw.hpp"
#include "solver.hpp"
#include "graph.hpp"
#include "logging.hpp"

namespace ratio
{
    atom_flaw::atom_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>> &&arguments) noexcept : flaw(s, std::move(causes)), atm(std::make_shared<atom>(pred, is_fact, *this, std::move(arguments))) {}

    void atom_flaw::compute_resolvers()
    {
        assert(get_solver().get_sat().value(get_phi()) != utils::False);        // The flaw is not necessarily false
        assert(get_solver().get_sat().value(atm->get_sigma()) != utils::False); // The atom is not necessarily inactive
        LOG_TRACE("Computing resolvers for " << to_json(*this).dump());
        // we check if the atom can unify..
        if (get_solver().get_sat().value(atm->get_sigma()) == utils::Undefined)
            for (auto &a : static_cast<riddle::predicate &>(atm->get_type()).get_atoms())
            {
                if (a == atm)
                    continue; // the current atom cannot unify with itself..
                if (!atm->get_reason().is_expanded())
                    continue; // the atom is not expanded yet, so we cannot unify it
                if (get_solver().get_idl_theory().distance(get_position(), static_cast<ratio::atom &>(*a).get_reason().get_position()).first > 0)
                    continue; // the unification would introduce a causal loop
                if (get_solver().get_sat().value(a->get_sigma()) == utils::False)
                    continue; // the atom is unified with another atom
                if (get_solver().get_sat().value(static_cast<atom &>(*a).get_reason().get_phi()) == utils::False)
                    continue; // the atom cannot be activated
                if (get_solver().matches(atm, a))
                    get_solver().get_graph().new_resolver<unify_atom>(*this, std::static_pointer_cast<atom>(a), get_solver().eq(atm, a)->get_value());
            }

        if (atm->is_fact())
            if (get_resolvers().empty())
                get_solver().get_graph().new_resolver<activate_fact>(*this, get_phi());
            else
                get_solver().get_graph().new_resolver<activate_fact>(*this);
        else if (get_resolvers().empty())
            get_solver().get_graph().new_resolver<activate_goal>(*this, get_phi());
        else
            get_solver().get_graph().new_resolver<activate_goal>(*this);
    }

    atom_flaw::activate_fact::activate_fact(atom_flaw &af) : resolver(af, utils::rational::zero) {}
    atom_flaw::activate_fact::activate_fact(atom_flaw &af, const utils::lit &rho) : resolver(af, rho, utils::rational::zero) {}

    void atom_flaw::activate_fact::apply()
    { // activating this resolver activates the fact..
        assert(get_flaw().get_solver().get_sat().value(static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()) != utils::False);
        assert(get_flaw().get_solver().get_sat().value(get_rho()) != utils::False);
        if (!get_flaw().get_solver().get_sat().new_clause({!get_rho(), static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()}))
            throw riddle::unsolvable_exception();
    }

    atom_flaw::activate_goal::activate_goal(atom_flaw &af) : resolver(af, utils::rational::zero) {}
    atom_flaw::activate_goal::activate_goal(atom_flaw &af, const utils::lit &rho) : resolver(af, rho, utils::rational::zero) {}

    void atom_flaw::activate_goal::apply()
    { // activating this resolver activates the goal..
        assert(get_flaw().get_solver().get_sat().value(static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()) != utils::False);
        assert(get_flaw().get_solver().get_sat().value(get_rho()) != utils::False);
        if (!get_flaw().get_solver().get_sat().new_clause({!get_rho(), static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()}))
            throw riddle::unsolvable_exception();

        // we call the corresponding rule..
        static_cast<riddle::predicate &>(static_cast<atom_flaw &>(get_flaw()).get_atom()->get_type()).call(static_cast<atom_flaw &>(get_flaw()).get_atom());
    }

    atom_flaw::unify_atom::unify_atom(atom_flaw &af, std::shared_ptr<atom> target, const utils::lit &unify_lit) : resolver(af, utils::rational::zero), target(target), unify_lit(unify_lit) {}

    void atom_flaw::unify_atom::apply()
    {
        assert(static_cast<atom_flaw &>(get_flaw()).get_atom()->get_reason().is_expanded());                                          // the current atom must be expanded..
        assert(get_flaw().get_solver().get_sat().value(static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()) != utils::True); // the current atom must be unifiable..
        assert(get_flaw().get_solver().get_sat().value(target->get_sigma()) != utils::False);                                         // the target atom must be activable..
        assert(get_flaw().get_solver().get_sat().value(get_rho()) != utils::False);                                                   // this resolver must be activable..

        // we add a causal link from the target atom's reason to this resolver..
        get_flaw().get_solver().get_graph().new_causal_link(get_flaw(), *this);

        for (const auto &res : get_flaw().get_resolvers())
            if (dynamic_cast<activate_fact *>(&res.get()) || dynamic_cast<activate_goal *>(&res.get()))
                if (!get_flaw().get_solver().get_sat().new_clause({!get_rho(), res.get().get_rho()})) // we disable this unification if the target atom is not activable..
                    throw riddle::unsolvable_exception();

        // as a consequence of the activation of this resolver:
        //  - we make the current atom's sigma false (unified atom)..
        if (!get_flaw().get_solver().get_sat().new_clause({!get_rho(), !static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()}))
            throw riddle::unsolvable_exception();
        //  - and we make the target atom's sigma true (active atom)..
        if (!get_flaw().get_solver().get_sat().new_clause({!get_rho(), target->get_sigma()}))
            throw riddle::unsolvable_exception();
        //  - and we constraint the current atom's and the target atom's variables to be pairwise equal..
        if (!get_flaw().get_solver().get_sat().new_clause({!get_rho(), unify_lit}))
            throw riddle::unsolvable_exception();
    }

#ifdef ENABLE_API
    json::json atom_flaw::get_data() const noexcept { return {{"type", "atom_flaw"}, {"atom", {{"id", get_id(*atm)}, {"is_fact", atm->is_fact()}, {"type", atm->get_type().get_name()}, {"sigma", variable(atm->get_sigma())}}}}; }

    json::json atom_flaw::activate_fact::get_data() const noexcept { return {{"type", "activate_fact"}}; }

    json::json atom_flaw::activate_goal::get_data() const noexcept { return {{"type", "activate_goal"}}; }

    json::json atom_flaw::unify_atom::get_data() const noexcept { return {{"type", "unify_atom"}, {"target", get_id(*target)}}; }
#endif
} // namespace ratio
