#pragma once

#include "types.hpp"

namespace ratio
{
  class basic_solver;

  class basic_component_type
  {
  public:
    basic_component_type(basic_solver &slv) noexcept;
    virtual ~basic_component_type() = default;

  protected:
    basic_solver &slv;
  };

  class basic_state_variable : public riddle::state_variable, public basic_component_type
  {
  public:
    basic_state_variable(basic_solver &slv) noexcept;
  };

  class basic_reusable_resource : public riddle::reusable_resource, public basic_component_type
  {
  public:
    basic_reusable_resource(basic_solver &slv) noexcept;
  };

  class basic_consumable_resource : public riddle::consumable_resource, public basic_component_type
  {
  public:
    basic_consumable_resource(basic_solver &slv) noexcept;
  };
} // namespace ratio
