#include <cassert>
#include "enum_flaw.hpp"
#include "solver.hpp"

namespace ratio
{
    enum_flaw::enum_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> item) noexcept : flaw(s, std::move(causes)), itm(item)
    {
        assert(itm != nullptr);
    }

    void enum_flaw::compute_resolvers()
    {
        throw std::runtime_error("Not implemented yet");
    }
} // namespace ratio
