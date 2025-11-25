#include "basic_types.hpp"
#include "basic_solver.hpp"

namespace ratio
{
    basic_component_type::basic_component_type(basic_solver &slv) noexcept : slv(slv) {}

    basic_state_variable::basic_state_variable(basic_solver &slv) noexcept : state_variable(slv), basic_component_type(slv) {}

    basic_reusable_resource::basic_reusable_resource(basic_solver &slv) noexcept : reusable_resource(slv), basic_component_type(slv) {}

    basic_consumable_resource::basic_consumable_resource(basic_solver &slv) noexcept : consumable_resource(slv), basic_component_type(slv) {}
} // namespace ratio