#pragma once

#include "z3solver.hpp"
#include "c++/z3++.h"

namespace ratio
{
  class z3flaw : public flaw
  {
  public:
    z3flaw(z3solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes) noexcept;

    [[nodiscard]] z3::expr &get_phi() noexcept { return phi; }
    [[nodiscard]] const z3::expr &get_phi() const noexcept { return phi; }

  private:
    static z3::expr compute_phi(z3solver &slv, const std::vector<std::reference_wrapper<resolver>> &causes) noexcept;

  private:
    z3::expr phi;
  };

  class z3resolver : public resolver
  {
  public:
    z3resolver(flaw &f, utils::rational &&intrinsic_cost, z3::expr &&rho) noexcept;

    [[nodiscard]] z3::expr &get_rho() noexcept { return rho; }
    [[nodiscard]] const z3::expr &get_rho() const noexcept { return rho; }

  private:
    z3::expr rho;
  };

  class z3atom_flaw : public z3flaw
  {
  public:
    z3atom_flaw(z3solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::atom_expr atom) noexcept;

    [[nodiscard]] riddle::atom_expr &get_atom() noexcept { return atom; }
    [[nodiscard]] const riddle::atom_expr &get_atom() const noexcept { return atom; }

  private:
    void compute_resolvers() override;

  private:
    riddle::atom_expr atom;
  };

  class z3disjunction_flaw : public z3flaw
  {
  public:
    z3disjunction_flaw(z3solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept;

    [[nodiscard]] const std::vector<std::unique_ptr<riddle::conjunction>> &get_disjuncts() const noexcept { return disjuncts; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<std::unique_ptr<riddle::conjunction>> disjuncts;
  };
} // namespace ratio
