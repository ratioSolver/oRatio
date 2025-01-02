#include "flaw.hpp"

namespace ratio
{
    flaw::flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes) : s(s), causes(causes) {}
} // namespace ratio