#pragma once

#include "flaw.h"
#include "resolver.h"

namespace ratio
{
  class disj_flaw final : public flaw
  {
  public:
    disj_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, std::vector<semitone::lit> lits);

  private:
    void compute_resolvers() override;

    json::json get_data() const noexcept override;

    class choose_lit final : public resolver
    {
    public:
      choose_lit(disj_flaw &ef, const utils::rational &cost, const semitone::lit &l);

      void apply() override {}

      json::json get_data() const noexcept override;
    };

  private:
    const std::vector<semitone::lit> lits; // the disjunction..
  };
} // namespace ratio
