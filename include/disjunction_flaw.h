#pragma once

#include "flaw.h"
#include "resolver.h"
#include "core_defs.h"
#include "conjunction.h"

namespace ratio::solver
{
  class disjunction_flaw final : public flaw
  {
  public:
    disjunction_flaw(solver &slv, std::vector<resolver *> causes, ratio::core::context ctx, std::vector<std::unique_ptr<ratio::core::conjunction>> conjs);
    disjunction_flaw(const disjunction_flaw &orig) = delete;

  private:
    void compute_resolvers() override;

    class choose_conjunction final : public resolver
    {
    public:
      choose_conjunction(disjunction_flaw &disj_flaw, ratio::core::context &ctx, std::unique_ptr<ratio::core::conjunction> conj);
      choose_conjunction(const choose_conjunction &that) = delete;

    private:
      void apply() override;

    private:
      ratio::core::context &ctx;                      // the context for executing the conjunction..
      std::unique_ptr<ratio::core::conjunction> conj; // the conjunction to execute..
    };

  private:
    ratio::core::context ctx;                                     // the context for executing the disjunction..
    std::vector<std::unique_ptr<ratio::core::conjunction>> conjs; // the disjunction to execute..
  };
} // namespace ratio::solver