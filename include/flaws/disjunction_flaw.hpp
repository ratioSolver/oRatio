#pragma once

#include "flaw.hpp"
#include "conjunction.hpp"

namespace ratio
{
  class disjunction_flaw : public flaw
  {
  public:
    disjunction_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&xprs) noexcept;

    [[nodiscard]] const std::vector<std::unique_ptr<riddle::conjunction>> &get_conjunctions() const noexcept { return xprs; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<std::unique_ptr<riddle::conjunction>> xprs;
  };
} // namespace ratio
