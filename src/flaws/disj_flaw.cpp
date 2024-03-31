#include <cassert>
#include "disj_flaw.hpp"

namespace ratio
{
    disj_flaw::disj_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<utils::lit> &&lits) noexcept : flaw(s, std::move(causes)), lits(lits)
    {
        assert(!lits.empty());
    }
} // namespace ratio
