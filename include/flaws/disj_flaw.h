#pragma once

#include "flaw.h"
#include "resolver.h"

namespace ratio::solver
{
  class disj_flaw final : public flaw
  {
  public:
    disj_flaw(solver &slv, std::vector<resolver *> causes, std::vector<semitone::lit> lits);
    disj_flaw(const disj_flaw &orig) = delete;

    ORATIO_EXPORT std::string get_data() const noexcept override;

  private:
    void compute_resolvers() override;

    class choose_lit final : public resolver
    {
    public:
      choose_lit(semitone::rational cst, disj_flaw &disj_flaw, const semitone::lit &p);
      choose_lit(const choose_lit &that) = delete;

      ORATIO_EXPORT std::string get_data() const noexcept override;

    private:
      void apply() override;
    };

  private:
    std::vector<semitone::lit> lits; // the disjunction..
  };
} // namespace ratio::solver