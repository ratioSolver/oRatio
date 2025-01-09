#pragma once

#include "z3solver.hpp"

namespace ratio
{
  constexpr const char *state_variable_kw = "state_variable";
  constexpr const char *reusable_resource_kw = "reusable_resource";
  constexpr const char *reusable_resource_capacity_kw = "capacity";
  constexpr const char *reusable_resource_amount_kw = "amount";

  class z3component_type : public riddle::component_type
  {
  public:
    z3component_type(z3solver &slv, std::string &&name) noexcept;
    virtual ~z3component_type() = default;

    virtual bool solve_inconsistencies() = 0;

  protected:
    void add(const std::vector<atom *> &atms, const z3::expr &e);
  };

  class z3state_variable final : public z3component_type
  {
  public:
    z3state_variable(z3solver &slv) noexcept;
    virtual ~z3state_variable() = default;

    bool solve_inconsistencies() override;
  };

  class z3reusable_resource final : public z3component_type
  {
  public:
    z3reusable_resource(z3solver &slv) noexcept;
    virtual ~z3reusable_resource() = default;

    bool solve_inconsistencies() override;
  };
} // namespace ratio
