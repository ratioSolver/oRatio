#pragma once

#include "z3solver.hpp"

namespace ratio
{
  constexpr const char *state_variable_kw = "state_variable";
  constexpr const char *reusable_resource_kw = "reusable_resource";

  class z3component_type : public riddle::component_type
  {
  public:
    z3component_type(z3solver &slv, std::string &&name) noexcept;
    virtual ~z3component_type() = default;

    virtual void solve_inconsistencies() = 0;
  };

  class z3state_variable final : public z3component_type
  {
  public:
    z3state_variable(z3solver &slv) noexcept;
    virtual ~z3state_variable() = default;

    void solve_inconsistencies() override;
  };

  class z3reusable_resource final : public z3component_type
  {
  public:
    z3reusable_resource(z3solver &slv) noexcept;
    virtual ~z3reusable_resource() = default;

    void solve_inconsistencies() override;
  };
} // namespace ratio
