#include "sttypes.hpp"

namespace ratio
{
    stcomponent_type::stcomponent_type(stsolver &slv, std::string &&name) noexcept : component_type(slv, std::move(name)) {}

    ststate_variable::ststate_variable(stsolver &slv) noexcept : stcomponent_type(slv, state_variable_kw) {}

    bool ststate_variable::solve_inconsistencies() {}

    void ststate_variable::created_predicate(riddle::predicate &pred) { add_parent(pred, get_core().get_predicate(interval_kw)); }

    streusable_resource::streusable_resource(stsolver &slv) noexcept : stcomponent_type(slv, reusable_resource_kw) {}

    bool streusable_resource::solve_inconsistencies() {}

    void streusable_resource::created_predicate(riddle::predicate &pred) { add_parent(pred, get_core().get_predicate(interval_kw)); }

    stconsumable_resource::stconsumable_resource(stsolver &slv) noexcept : stcomponent_type(slv, consumable_resource_kw) {}

    bool stconsumable_resource::solve_inconsistencies() {}

    void stconsumable_resource::created_predicate(riddle::predicate &pred) { add_parent(pred, get_core().get_predicate(interval_kw)); }
} // namespace ratio
