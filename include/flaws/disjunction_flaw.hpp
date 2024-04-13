#pragma once

#include "flaw.hpp"
#include "conjunction.hpp"
#include "resolver.hpp"

namespace ratio
{
  class disjunction_flaw final : public flaw
  {
  public:
    disjunction_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&conjs) noexcept;

    [[nodiscard]] const std::vector<std::unique_ptr<riddle::conjunction>> &get_conjunctions() const noexcept { return conjs; }

  private:
    void compute_resolvers() override;

    class choose_conjunction final : public resolver
    {
    public:
      choose_conjunction(disjunction_flaw &df, riddle::conjunction &conj, const utils::rational &cost);

      void apply() override;

    private:
      riddle::conjunction &conj;
    };

  private:
    std::vector<std::unique_ptr<riddle::conjunction>> conjs;
  };
} // namespace ratio
