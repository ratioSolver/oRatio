#pragma once

#include "types.hpp"
#include "semitone.hpp"
#include "la_theory.hpp"

namespace ratio
{
  class solver;
  class atom;
  class resolver;
  class stcomponent_type;

  class atom_listener : smt::prop_listener, smt::la_listener
  {
  public:
    atom_listener(stcomponent_type &ct, atom &atm) noexcept;

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
    friend class atom_listener;

  public:
    stcomponent_type(solver &slv) noexcept;
    virtual ~stcomponent_type() = default;

    virtual std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() noexcept = 0;

    solver &get_solver() const { return slv; }

  protected:
    utils::lbool share_component(riddle::atom_expr lhs, riddle::atom_expr rhs);

    void new_flaw(std::vector<utils::ref_wrapper<resolver>> &&causes, std::vector<utils::lit> &&clause);

  private:
    solver &slv; // the solver..

  protected:
    std::set<const riddle::component *> to_check;                                                             // the components whose atoms have changed..
    std::vector<std::pair<std::vector<utils::ref_wrapper<resolver>>, std::vector<utils::lit>>> pending_flaws; // the pending flaws..
  };

  class ststate_variable final : public riddle::state_variable, public stcomponent_type
  {
  public:
    ststate_variable(solver &slv) noexcept;

    std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() noexcept override;

    void created_atom(riddle::atom_expr atm) noexcept override;

  private:
    std::set<std::set<riddle::atom_term *>> sv_flaws;                              // the state-variable flaws found so far..
    std::map<riddle::atom_term *, std::map<riddle::atom_term *, utils::lit>> leqs; // all the possible ordering constraints..
    std::map<riddle::atom_term *, std::map<utils::enum_val *, utils::lit>> frbs;   // all the possible forbidding constraints..
  };

  class streusable_resource final : public riddle::reusable_resource, public stcomponent_type
  {
  public:
    streusable_resource(solver &slv) noexcept;

    std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() noexcept override;

    void created_atom(riddle::atom_expr atm) noexcept override;

  private:
    std::map<riddle::atom_term *, std::map<riddle::atom_term *, utils::lit>> leqs; // all the possible ordering constraints..
    std::map<riddle::atom_term *, std::map<utils::enum_val *, utils::lit>> frbs;   // all the possible forbidding constraints..
  };

  class stconsumable_resource final : public riddle::consumable_resource, public stcomponent_type
  {
  public:
    stconsumable_resource(solver &slv) noexcept;

    std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() noexcept override;

    void created_atom(riddle::atom_expr atm) noexcept override;

  private:
    std::map<riddle::atom_term *, std::map<riddle::atom_term *, utils::lit>> leqs; // all the possible ordering constraints..
    std::map<riddle::atom_term *, std::map<utils::enum_val *, utils::lit>> frbs;   // all the possible forbidding constraints..
  };
} // namespace ratio