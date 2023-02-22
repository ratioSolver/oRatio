#pragma once

#include "flaw.h"
#include "resolver.h"
#include "items.h"
#include "var_value.h"

namespace ratio
{
  class enum_flaw final : public flaw
  {
  public:
    enum_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, enum_item &ei);

    void compute_resolvers() override;

    json::json get_data() const noexcept override;

    class enum_resolver final : public resolver
    {
    public:
      enum_resolver(enum_flaw &ef, const utils::rational &cost, semitone::var_value &val);

      void apply() override;

      json::json get_data() const noexcept override;

    private:
      semitone::var_value &val;
    };

  private:
    enum_item &ei;
  };
} // namespace ratio
