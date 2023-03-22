#pragma once

#include "flaw.h"
#include "resolver.h"
#include "enum.h"

namespace ratio
{
  class enum_item;

  class enum_flaw final : public flaw
  {
  public:
    enum_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, enum_item &ei);

  private:
    void compute_resolvers() override;

    json::json get_data() const noexcept override;

    class enum_resolver final : public resolver
    {
    public:
      enum_resolver(enum_flaw &ef, const semitone::lit &rho, const utils::rational &cost, utils::enum_val &val);

      void apply() override {}

      json::json get_data() const noexcept override;

    private:
      utils::enum_val &val;
    };

  private:
    enum_item &ei;
  };
} // namespace ratio
