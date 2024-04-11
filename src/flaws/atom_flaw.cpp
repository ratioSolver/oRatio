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
} // namespace ratio
