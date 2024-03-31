#pragma once

#include "flaw.hpp"
#include "item.hpp"

namespace ratio
{
  class bool_flaw : public flaw
  {
  public:
    bool_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::bool_item> b_item) noexcept;

    [[nodiscard]] const std::shared_ptr<riddle::bool_item> &get_bool_item() const noexcept { return b_item; }

  private:
    std::shared_ptr<riddle::bool_item> b_item;
  };
} // namespace ratio
