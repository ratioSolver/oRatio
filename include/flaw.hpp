#pragma once

#include <vector>
#include <functional>

namespace ratio
{
  class solver;
  class resolver;

  class flaw
  {
    friend class resolver;

  public:
    flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes);

    [[nodiscard]] solver &get_solver() noexcept { return s; }
    [[nodiscard]] const solver &get_solver() const noexcept { return s; }

    [[nodiscard]] const std::vector<std::reference_wrapper<resolver>> get_causes() const noexcept { return causes; }

    [[nodiscard]] const std::vector<std::reference_wrapper<resolver>> get_resolvers() const noexcept { return resolvers; }

  private:
    solver &s;                                               // the solver for this flaw..
    std::vector<std::reference_wrapper<resolver>> causes;    // the causes of this flaw..
    std::vector<std::reference_wrapper<resolver>> resolvers; // the resolvers for this flaw..
  };
} // namespace ratio
