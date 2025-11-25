#pragma once

#include "types.hpp"

namespace ratio
{
  class solver;

  class basic_component_type
  {
  public:
    basic_component_type(solver &slv) noexcept;
    virtual ~basic_component_type() = default;

  protected:
    solver &slv;
  };

  class basic_state_variable : public riddle::state_variable, public basic_component_type
  {
  public:
    basic_state_variable(solver &slv) noexcept;
  };

  class basic_reusable_resource : public riddle::reusable_resource, public basic_component_type
  {
  public:
    basic_reusable_resource(solver &slv) noexcept;
  };

  class basic_consumable_resource : public riddle::consumable_resource, public basic_component_type
  {
  public:
    basic_consumable_resource(solver &slv) noexcept;
  };
} // namespace ratio
