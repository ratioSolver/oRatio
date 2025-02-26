#pragma once

#include "stsolver.hpp"

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

  class stcomponent_type : public riddle::component_type
  {
  public:
    stcomponent_type(stsolver &slv, std::string &&name) noexcept;

    virtual bool solve_inconsistencies() = 0;
  };

  class ststate_variable final : public stcomponent_type
  {
  public:
    ststate_variable(stsolver &slv) noexcept;
    virtual ~ststate_variable() = default;

    bool solve_inconsistencies() override;

  private:
    void created_predicate(riddle::predicate &pred) override;
  };

  class streusable_resource final : public stcomponent_type
  {
  public:
    streusable_resource(stsolver &slv) noexcept;
    virtual ~streusable_resource() = default;

    bool solve_inconsistencies() override;

  private:
    void created_predicate(riddle::predicate &pred) override;
  };

  class stconsumable_resource final : public stcomponent_type
  {
  public:
    stconsumable_resource(stsolver &slv) noexcept;
    virtual ~stconsumable_resource() = default;

    bool solve_inconsistencies() override;

  private:
    void created_predicate(riddle::predicate &pred) override;
  };
} // namespace ratio