#include "flaws.hpp"
#include <cassert>

namespace ratio
{
    enum_flaw::enum_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> var) noexcept : flaw(slv, std::move(causes)), var(std::move(var)) {}
} // namespace ratio
