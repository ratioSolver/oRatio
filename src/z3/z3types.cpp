#include "z3types.hpp"

namespace ratio
{
    z3component_type::z3component_type(z3solver &slv, std::string &&name) noexcept : riddle::component_type(slv, std::move(name)) {}

    z3state_variable::z3state_variable(z3solver &slv) noexcept : z3component_type(slv, state_variable_kw) {}

    void z3state_variable::solve_inconsistencies() {}

    z3reusable_resource::z3reusable_resource(z3solver &slv) noexcept : z3component_type(slv, reusable_resource_kw) {}

    void z3reusable_resource::solve_inconsistencies() {}
} // namespace ratio