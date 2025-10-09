#pragma once

#include "rational.hpp"
#include <vector>
#include <functional>

namespace ratio
{
  class solver;
  class resolver;

  class flaw
  {
  public:
    flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, const bool &exclusive = false);
    flaw(const flaw &) = delete;
    virtual ~flaw() = default;
  };

  class resolver
  {
  public:
    resolver(flaw &f, utils::rational &&intrinsic_cost);
    resolver(const resolver &) = delete;
    virtual ~resolver() = default;
  };
} // namespace ratio
