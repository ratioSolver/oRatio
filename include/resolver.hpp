#pragma once

#include "rational.hpp"

namespace ratio
{
  class flaw;

  class resolver
  {
  public:
    resolver(flaw &f, utils::rational &&intrinsic_cost);

    [[nodiscard]] flaw &get_flaw() noexcept { return f; }
    [[nodiscard]] const flaw &get_flaw() const noexcept { return f; }

    [[nodiscard]] const utils::rational &get_intrinsic_cost() const noexcept { return intrinsic_cost; }

  private:
    flaw &f;
    utils::rational intrinsic_cost;
  };
} // namespace ratio
