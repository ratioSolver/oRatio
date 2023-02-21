#pragma once

#include "solver.h"

namespace ratio
{
  class solver_listener
  {
  public:
    solver_listener(solver &s) : s(s) { s.listeners.push_back(this); }
    virtual ~solver_listener() { slv.listeners.erase(std::find(slv.listeners.cbegin(), slv.listeners.cend(), this)); }

  private:
    solver &s;
  };
} // namespace ratio
