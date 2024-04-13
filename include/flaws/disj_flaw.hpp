#pragma once

#include "flaw.hpp"

namespace ratio
{
  class disj_flaw : public flaw
  {
  public:
    disj_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<utils::lit> &&lits, bool exclusive = false) noexcept;

    [[nodiscard]] const std::vector<utils::lit> &get_lits() const noexcept { return lits; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<utils::lit> lits;
  };
} // namespace ratio
