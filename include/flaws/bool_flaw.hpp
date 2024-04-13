#pragma once

#include "flaw.hpp"
#include "resolver.hpp"

namespace ratio
{
  class bool_flaw final : public flaw
  {
  public:
    bool_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, const utils::lit &l) noexcept;

    [[nodiscard]] const utils::lit &get_lit() const { return l; }

  private:
    void compute_resolvers() override;

    class choose_value final : public resolver
    {
    public:
      choose_value(bool_flaw &bf, const utils::rational &cost, const utils::lit &l);

      void apply() override {}
    };

  private:
    utils::lit l;
  };
} // namespace ratio
