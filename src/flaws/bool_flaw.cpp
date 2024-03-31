#include <cassert>
#include "bool_flaw.hpp"

namespace ratio
{
    bool_flaw::bool_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::bool_item> b_item) noexcept : flaw(s, std::move(causes)), b_item(b_item)
    {
        assert(b_item);
    }
} // namespace ratio
