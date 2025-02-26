#pragma once

#include "graph.hpp"

namespace ratio
{
  constexpr const char *state_variable_kw = "StateVariable";
  constexpr const char *reusable_resource_kw = "ReusableResource";
  constexpr const char *reusable_resource_capacity_kw = "capacity";
  constexpr const char *reusable_resource_amount_kw = "amount";
  constexpr const char *consumable_resource_kw = "ConsumableResource";
  constexpr const char *consumable_resource_capacity_kw = "capacity";
  constexpr const char *consumable_resource_initial_amount_kw = "initial_amount";
  constexpr const char *consumable_resource_amount_kw = "amount";

  class timeline
  {
  public:
    timeline() = default;
    virtual ~timeline() = default;

    [[nodiscard]] virtual json::json extract() const = 0;
  };

  class state_variable : public riddle::component_type, public timeline
  {
  public:
    state_variable(graph &slv) noexcept;

    [[nodiscard]] virtual json::json extract() const override;

  private:
    void created_predicate(riddle::predicate &pred) override;
  };

  class reusable_resource : public riddle::component_type, public timeline
  {
  public:
    reusable_resource(graph &slv) noexcept;

    [[nodiscard]] virtual json::json extract() const override;

  private:
    utils::u_ptr<riddle::constructor_declaration> ctr;
    utils::u_ptr<riddle::predicate_declaration> use_pred;
  };

  class consumable_resource : public riddle::component_type, public timeline
  {
  public:
    consumable_resource(graph &slv) noexcept;

    [[nodiscard]] virtual json::json extract() const override;

  private:
    utils::u_ptr<riddle::constructor_declaration> ctr;
    utils::u_ptr<riddle::predicate_declaration> prod_pred;
    utils::u_ptr<riddle::predicate_declaration> cons_pred;
  };
} // namespace ratio