#pragma once

#include "flaw.hpp"
#include "item.hpp"

namespace ratio
{
  class enum_flaw : public flaw
  {
  public:
    enum_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> item) noexcept;

    [[nodiscard]] const std::shared_ptr<riddle::enum_item> &get_item() const noexcept { return itm; }

  private:
    void compute_resolvers() override;

#ifdef ENABLE_VISUALIZATION
    json::json get_data() const noexcept override;
#endif

  private:
    std::shared_ptr<riddle::enum_item> itm;
  };
} // namespace ratio
