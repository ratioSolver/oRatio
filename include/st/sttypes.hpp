#pragma once

#include "types.hpp"

namespace ratio
{
  class stsolver;

  class stcomponent_type
  {
  public:
    virtual ~stcomponent_type() = default;

    virtual bool solve_inconsistencies() = 0;
  };

  class ststate_variable final : public state_variable, public stcomponent_type
  {
  public:
    ststate_variable(stsolver &slv) noexcept;

    bool solve_inconsistencies() override;
  };

  class streusable_resource final : public reusable_resource, public stcomponent_type
  {
  public:
    streusable_resource(stsolver &slv) noexcept;

    bool solve_inconsistencies() override;
  };

  class stconsumable_resource final : public consumable_resource, public stcomponent_type
  {
  public:
    stconsumable_resource(stsolver &slv) noexcept;

    bool solve_inconsistencies() override;
  };
} // namespace ratio