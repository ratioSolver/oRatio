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
    disjunction_flaw(solver &slv, std::vector<resolver *> causes, std::vector<std::unique_ptr<ratio::core::conjunction>> conjs);
    disjunction_flaw(const disjunction_flaw &orig) = delete;

    ORATIO_EXPORT json::json get_data() const noexcept override;

  private:
    void compute_resolvers() override;

    class choose_conjunction final : public resolver
    {
    public:
      choose_conjunction(disjunction_flaw &disj_flaw, std::unique_ptr<ratio::core::conjunction> conj);
      choose_conjunction(const choose_conjunction &that) = delete;

      ORATIO_EXPORT json::json get_data() const noexcept override;

    private:
      void apply() override;

    private:
      std::unique_ptr<ratio::core::conjunction> conj; // the conjunction to execute..
    };

  private:
    std::vector<std::unique_ptr<ratio::core::conjunction>> conjs; // the disjunction to execute..
  };
} // namespace ratio::solver