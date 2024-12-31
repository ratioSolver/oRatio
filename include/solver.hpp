#pragma once

#if defined(SEMITONE)
#include "semitonecore.hpp"
#elif defined(Z3)
#include "z3core.hpp"
#endif

namespace ratio
{
  class solver : public core
  {
  public:
    solver();
  };
} // namespace ratio
