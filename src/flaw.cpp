#include "flaw.hpp"
#include "solver.hpp"

namespace ratio
{
    flaw::flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, const bool &exclusive) noexcept : s(s), causes(causes), exclusive(exclusive), position(s.get_idl_theory().new_var()) {}
} // namespace ratio
