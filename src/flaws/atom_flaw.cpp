#include <cassert>
#include "atom_flaw.hpp"
#include "solver.hpp"

namespace ratio
{
    atom_flaw::atom_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>> &&arguments) noexcept : flaw(s, std::move(causes)), atm(std::make_shared<atom>(pred, is_fact, *this, std::move(arguments))) {}

    void atom_flaw::compute_resolvers()
    {
        throw std::runtime_error("Not implemented yet");
    }

    atom_flaw::activate_fact::activate_fact(atom_flaw &af) : resolver(af, utils::rational::zero) {}

    void atom_flaw::activate_fact::apply()
    {
        throw std::runtime_error("Not implemented yet");
    }

    atom_flaw::activate_goal::activate_goal(atom_flaw &af) : resolver(af, utils::rational::zero) {}

    void atom_flaw::activate_goal::apply()
    {
        throw std::runtime_error("Not implemented yet");
    }

    atom_flaw::unify_atom::unify_atom(atom_flaw &af, std::shared_ptr<atom> target, const utils::lit &unify_lit) : resolver(af, utils::rational::zero), target(target), unify_lit(unify_lit) {}

    void atom_flaw::unify_atom::apply()
    {
        throw std::runtime_error("Not implemented yet");
    }

#ifdef ENABLE_VISUALIZATION
    json::json atom_flaw::get_data() const noexcept { return {{"type", "atom_flaw"}, {"atom", {{"id", get_id(*atm)}, {"is_fact", atm->is_fact()}, {"type", atm->get_type().get_name()}, {"sigma", variable(atm->get_sigma())}}}}; }
#endif
} // namespace ratio
