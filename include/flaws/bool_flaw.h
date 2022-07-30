#pragma once

#include "flaw.h"
#include "resolver.h"
#include "item.h"

namespace ratio::solver
{
  class bool_flaw final : public flaw
  {
  public:
    bool_flaw(solver &slv, std::vector<resolver *> causes, ratio::core::bool_item &b_itm);
    bool_flaw(const bool_flaw &orig) = delete;

  private:
    void compute_resolvers() override;

    class choose_value final : public resolver
    {
    public:
      choose_value(semitone::rational cst, bool_flaw &bl_flaw, const semitone::lit &val);
      choose_value(const choose_value &that) = delete;

    private:
      void apply() override;
    };

  private:
    ratio::core::bool_item &b_itm; // the bool variable whose value has to be decided..
  };
} // namespace ratio::solver