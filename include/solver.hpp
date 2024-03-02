#pragma once

#include "solver_item.hpp"
#include "core.hpp"
#include "sat_core.hpp"
#include "lra_theory.hpp"

namespace ratio
{
  class solver : public riddle::core
  {
  public:
    solver(const std::string &name = "oRatio", bool i = true) noexcept;
    virtual ~solver() = default;

    void init() noexcept;

  private:
    std::string name;
  };
} // namespace ratio
