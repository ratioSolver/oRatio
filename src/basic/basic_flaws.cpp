#include "basic_flaws.hpp"
#include "items.hpp"

namespace ratio
{
    flaw::flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes) : riddle::flaw(slv, std::move(causes)) {}

    resolver::resolver(flaw &flw, utils::rational &&intrinsic_cost) : riddle::resolver(flw, std::move(intrinsic_cost)) {}

    enum_flaw::enum_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev) noexcept : flaw(slv, std::move(causes)), var(std::make_shared<riddle::enum_item>(*this, tp, std::move(values), ev)) {}

    utils::rational enum_flaw::get_estimated_cost() const noexcept { return get_core().enum_value(*var).size(); }
} // namespace ratio
