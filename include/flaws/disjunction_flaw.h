#pragma once

#include "flaw.h"
#include "resolver.h"
#include "conjunction.h"

namespace ratio
{
  class disjunction_flaw final : public flaw
  {
  public:
    disjunction_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, std::vector<riddle::conjunction_ptr> xprs);

  private:
    void compute_resolvers() override;

    json::json get_data() const noexcept override;

    class choose_conjunction final : public resolver
    {
    public:
      choose_conjunction(disjunction_flaw &df, const riddle::conjunction_ptr &xpr);

      void apply() override;

      json::json get_data() const noexcept override;

    private:
      const riddle::conjunction_ptr &xpr;
    };

  private:
    std::vector<riddle::conjunction_ptr> xprs; // the disjunction..
  };
} // namespace ratio
