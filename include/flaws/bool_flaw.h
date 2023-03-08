#pragma once

#include "flaw.h"
#include "resolver.h"
#include "item.h"

namespace ratio
{
  class bool_flaw final : public flaw
  {
  public:
    bool_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, riddle::expr &b_item);

  private:
    void compute_resolvers() override;

    json::json get_data() const noexcept override;

    riddle::expr &get_bool_item() noexcept { return b_item; }

    class choose_value final : public resolver
    {
    public:
      choose_value(bool_flaw &ef, const semitone::lit &rho);

      void apply() override;

      json::json get_data() const noexcept override;
    };

  private:
    riddle::expr b_item;
  };
} // namespace ratio
