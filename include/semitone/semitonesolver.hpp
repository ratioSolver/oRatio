#pragma once

#include "graph.hpp"

namespace ratio
{
  class solver : public graph
  {
  public:
    solver(std::string_view name = "oRatio") noexcept;
    virtual ~solver() = default;
  };
} // namespace ratio
