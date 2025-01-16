#pragma once

#include "json.hpp"

namespace ratio
{
  class graph;

  namespace server
  {
    [[nodiscard]] json::json make_solver_message(const graph &g) noexcept;

    [[nodiscard]] json::json make_solvers_message(const std::vector<std::reference_wrapper<graph>> &gs) noexcept;
  } // namespace server
} // namespace ratio
