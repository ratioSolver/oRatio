#pragma once

#include "flaw.h"
#include "resolver.h"
#include "item.h"

namespace ratio::solver
{
  class enum_flaw final : public flaw
  {
  public:
    enum_flaw(solver &slv, std::vector<resolver *> causes, ratio::core::enum_item &v_itm);
    enum_flaw(const enum_flaw &orig) = delete;

    ORATIO_EXPORT json::json get_data() const noexcept override;

  private:
    void compute_resolvers() override;

    class choose_value final : public resolver
    {
    public:
      choose_value(semitone::rational cst, enum_flaw &enm_flaw, semitone::var_value &val);
      choose_value(const choose_value &that) = delete;

      ORATIO_EXPORT json::json get_data() const noexcept override;

    private:
      void apply() override;

    private:
      semitone::var v;          // the object variable whose value has to be decided..
      semitone::var_value &val; // the decided value..
    };

  private:
    ratio::core::enum_item &v_itm; // the enum variable whose value has to be decided..
  };
} // namespace ratio::solver