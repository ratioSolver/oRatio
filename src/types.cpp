#include "types.hpp"
#include <sstream>

namespace ratio
{
    state_variable::state_variable(graph &gr) noexcept : component_type(gr, state_variable_kw) { add_constructor(utils::make_u_ptr<riddle::constructor>(*this)); }

    void state_variable::created_predicate(riddle::predicate &pred) { add_parent(pred, get_core().get_predicate(interval_kw)); }

    json::json state_variable::extract() const {}

    reusable_resource::reusable_resource(graph &gr) noexcept : component_type(gr, reusable_resource_kw)
    {
        add_field(utils::make_u_ptr<riddle::field>(gr.get_type(riddle::real_kw), reusable_resource_capacity_kw, nullptr));

        std::istringstream ctr_decl("ReusableResource(real capacity) : capacity(capacity) { capacity >= 0.0; }");
        riddle::parser ctr_p(ctr_decl);
        ctr = ctr_p.parse_constructor_declaration();
        ctr->refine(*this);

        std::istringstream use_pred_decl("predicate Use(real amount) : Interval { amount >= 0.0; }");
        riddle::parser use_pred_p(use_pred_decl);
        use_pred = use_pred_p.parse_predicate_declaration();
        use_pred->declare(*this);
        use_pred->refine(*this);
    }

    json::json reusable_resource::extract() const {}

    consumable_resource::consumable_resource(graph &gr) noexcept : component_type(gr, consumable_resource_kw)
    {
        add_field(utils::make_u_ptr<riddle::field>(gr.get_type(riddle::real_kw), consumable_resource_capacity_kw, nullptr));
        add_field(utils::make_u_ptr<riddle::field>(gr.get_type(riddle::real_kw), consumable_resource_initial_amount_kw, nullptr));

        std::istringstream ctr_decl("ConsumableResource(real capacity, real initial_amount) : capacity(capacity), initial_amount(initial_amount) { capacity >= 0.0; initial_amount <= capacity; }");
        riddle::parser ctr_p(ctr_decl);
        ctr = ctr_p.parse_constructor_declaration();
        ctr->refine(*this);

        std::istringstream prod_pred_decl("predicate Produce(real amount) : Interval { amount >= 0.0; }");
        riddle::parser prod_pred_p(prod_pred_decl);
        prod_pred = prod_pred_p.parse_predicate_declaration();
        prod_pred->declare(*this);
        prod_pred->refine(*this);

        std::istringstream cons_pred_decl("predicate Consume(real amount) : Interval { amount >= 0.0; }");
        riddle::parser cons_pred_p(cons_pred_decl);
        cons_pred = cons_pred_p.parse_predicate_declaration();
        cons_pred->declare(*this);
        cons_pred->refine(*this);
    }

    json::json consumable_resource::extract() const {}
} // namespace ratio
