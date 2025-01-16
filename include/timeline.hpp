#pragma once

#include "json.hpp"

namespace ratio
{
  class timeline
  {
  public:
    timeline() = default;
    virtual ~timeline() = default;

    [[nodiscard]] virtual json::json extract() const = 0;
  };
} // namespace ratio
