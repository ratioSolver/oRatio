#pragma once

#include "solver_core.hpp"

namespace ratio
{
  class basic_solver : public solver_core
  {
  public:
    basic_solver() noexcept;

    void solve() override;
  };
} // namespace ratio
