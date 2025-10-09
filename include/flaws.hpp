#pragma once

#include "graph.hpp"
#include "items.hpp"

namespace ratio
{
  class enum_flaw final : public flaw
  {
  public:
    enum_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> var) noexcept;

    [[nodiscard]] const std::shared_ptr<riddle::enum_item> &get_var() const noexcept { return var; }

  private:
    std::shared_ptr<riddle::enum_item> var;
  };
} // namespace ratio
