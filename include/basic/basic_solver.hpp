#pragma once

#include "solver.hpp"
#include "a_star.hpp"

namespace ratio
{
  class basic_solver : public solver, public utils::a_star<double>
  {
  };
} // namespace ratio
