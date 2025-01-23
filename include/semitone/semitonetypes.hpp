#pragma once

#include "semitonesolver.hpp"

namespace ratio
{
  constexpr const char *state_variable_kw = "StateVariable";
  constexpr const char *reusable_resource_kw = "ReusableResource";
  constexpr const char *reusable_resource_capacity_kw = "capacity";
  constexpr const char *reusable_resource_amount_kw = "amount";

  class semitonecomponent_type : public riddle::component_type
  {
  };
} // namespace ratio