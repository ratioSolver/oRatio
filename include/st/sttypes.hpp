#pragma once

#include "types.hpp"
#include "semitone.hpp"
#include "la_theory.hpp"

namespace ratio
{
  class solver;
  class atom;
  class stcomponent_type;

  class statom_listener : smt::prop_listener, smt::la_listener
  {
  public:
    statom_listener(stcomponent_type &ct, atom &atm) noexcept;

  private:
    void on_change(const utils::var &v) noexcept override;
    void on_reset(const utils::var &) noexcept override {}
    void on_arith_change(const utils::var &v) noexcept override;

  private:
    stcomponent_type &ct;
    atom &atm;
  };

  class stcomponent_type
  {
    friend class statom_listener;

  public:
    virtual ~stcomponent_type() = default;

    virtual bool solve_inconsistencies() = 0;

  private:
    std::set<const riddle::component *> to_check; // the components whose atoms have changed..
  };

  class ststate_variable final : public riddle::state_variable, public stcomponent_type
  {
  public:
    ststate_variable(solver &slv) noexcept;

    bool solve_inconsistencies() override;
  };

  class streusable_resource final : public riddle::reusable_resource, public stcomponent_type
  {
  public:
    streusable_resource(solver &slv) noexcept;

    bool solve_inconsistencies() override;
  };

  class stconsumable_resource final : public riddle::consumable_resource, public stcomponent_type
  {
  public:
    stconsumable_resource(solver &slv) noexcept;

    bool solve_inconsistencies() override;
  };
} // namespace ratio