#pragma once

#include "flaw.hpp"
#include "item.hpp"
#include "resolver.hpp"

namespace ratio
{
  class enum_flaw : public flaw
  {
  public:
    enum_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> item) noexcept;

    [[nodiscard]] const std::shared_ptr<riddle::enum_item> &get_item() const noexcept { return itm; }

  private:
    void compute_resolvers() override;

    class choose_value final : public resolver
    {
    public:
      choose_value(enum_flaw &bf, const utils::lit &rho, const utils::rational &cost, const utils::enum_val &val);

      void apply() override {}

#ifdef ENABLE_VISUALIZATION
      json::json get_data() const noexcept override;
#endif

    private:
      const utils::enum_val &val;
    };

#ifdef ENABLE_VISUALIZATION
    json::json get_data() const noexcept override;
#endif

  private:
    std::shared_ptr<riddle::enum_item> itm;
  };
} // namespace ratio
