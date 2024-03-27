#include <algorithm>
#include "flaw.hpp"
#include "solver.hpp"

namespace ratio
{
    flaw::flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, const bool &exclusive) noexcept : s(s), causes(causes), exclusive(exclusive), position(s.get_idl_theory().new_var()) {}

    resolver &flaw::get_cheapest_resolver() const noexcept
    {
        return *std::min_element(resolvers.begin(), resolvers.end(), [](const auto &lhs, const auto &rhs)
                                 { return lhs.get().get_estimated_cost() < rhs.get().get_estimated_cost(); });
    }

    void flaw::init() noexcept
    {
    }
} // namespace ratio
