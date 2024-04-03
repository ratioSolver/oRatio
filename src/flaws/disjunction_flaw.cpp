#include <cassert>
#include "disjunction_flaw.hpp"

namespace ratio
{
    disjunction_flaw::disjunction_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::reference_wrapper<riddle::conjunction>> &&xprs) noexcept : flaw(s, std::move(causes)), xprs(std::move(xprs))
    {
        assert(!xprs.empty());
    }
} // namespace ratio
