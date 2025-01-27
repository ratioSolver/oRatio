#pragma once

#include "z3solver.hpp"
#include "c++/z3++.h"

namespace ratio
{
  class z3flaw : public flaw
  {
  public:
    z3flaw(z3solver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes) noexcept;

    [[nodiscard]] z3::expr get_phi() const noexcept { return phi; }

    [[nodiscard]] z3::expr get_pos() const noexcept { return pos; }

  private:
    [[nodiscard]] static z3::expr compute_phi(z3solver &slv, const std::vector<utils::ref_wrapper<resolver>> &causes) noexcept;

    void expanded_flaw() override;

  protected:
    [[nodiscard]] json::json to_json() const override;

  private:
    z3::expr phi; // the literal indicating whether the flaw is active or not..
    z3::expr pos; // the position variable associated to this flaw (for avoiding causality loops)..
  };

  class z3resolver : public resolver
  {
  public:
    z3resolver(flaw &f, utils::rational &&intrinsic_cost) noexcept;
    z3resolver(flaw &f, utils::rational &&intrinsic_cost, z3::expr rho) noexcept;

    [[nodiscard]] z3::expr get_rho() const noexcept { return rho; }

  protected:
    void add(const z3::expr &e);

    [[nodiscard]] json::json to_json() const override;

  private:
    z3::expr rho;
  };

  class z3atom_flaw final : public z3flaw
  {
  public:
    z3atom_flaw(z3solver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args) noexcept;

    [[nodiscard]] atom_expr get_atom() const noexcept { return atm; }

  private:
    void compute_resolvers() override;

    json::json to_json() const override;

  private:
    atom_expr atm; // the atom that is the subject of the flaw..
  };

  class z3activate_fact final : public z3resolver
  {
  public:
    z3activate_fact(z3atom_flaw &f) noexcept;
    z3activate_fact(z3atom_flaw &f, z3::expr rho) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;
  };

  class z3activate_goal final : public z3resolver
  {
  public:
    z3activate_goal(z3atom_flaw &f) noexcept;
    z3activate_goal(z3atom_flaw &f, z3::expr rho) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;
  };

  class z3unify_atom final : public z3resolver
  {
  public:
    z3unify_atom(z3atom_flaw &f, atom_expr atm) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;

  private:
    atom_expr atm; // the atom to unify with..
  };

  class z3disjunction_flaw final : public z3flaw
  {
  public:
    z3disjunction_flaw(z3solver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts) noexcept;

    [[nodiscard]] const std::vector<utils::u_ptr<riddle::conjunction>> &get_disjuncts() const noexcept { return disjuncts; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<utils::u_ptr<riddle::conjunction>> disjuncts;
  };

  class z3choose_conjunction final : public z3resolver
  {
  public:
    z3choose_conjunction(z3disjunction_flaw &f, riddle::conjunction &conj) noexcept;

  private:
    void apply() override;

  private:
    riddle::conjunction &conj;
  };
} // namespace ratio
