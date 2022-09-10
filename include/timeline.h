#pragma once

#include "json.h"

namespace ratio::solver
{
  class timeline
  {
  public:
    virtual ~timeline() {}

    virtual json::json extract() const noexcept = 0;
  };
} // namespace ratio::solver
