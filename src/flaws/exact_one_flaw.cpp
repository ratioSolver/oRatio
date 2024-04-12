#include <cassert>
#include "exact_one_flaw.hpp"
#include "solver.hpp"

namespace ratio
{
    exact_one_flaw::exact_one_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<utils::lit> &&lits) noexcept : flaw(s, std::move(causes)), lits(lits)
    {
        assert(!lits.empty());
    }

    void exact_one_flaw::compute_resolvers()
    {
        throw std::runtime_error("Not implemented yet");
    }
} // namespace ratio
