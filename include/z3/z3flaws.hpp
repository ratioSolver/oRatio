#pragma once

#include "z3solver.hpp"
#include "c++/z3++.h"

namespace ratio
{
  class z3atom_flaw : public flaw
  {
  public:
    z3atom_flaw(graph &gr, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::atom_expr atom, z3::expr phi) noexcept;

    [[nodiscard]] z3::expr &get_phi() noexcept { return phi; }
    [[nodiscard]] const z3::expr &get_phi() const noexcept { return phi; }

  private:
    riddle::atom_expr atom;
    z3::expr phi;
  };
} // namespace ratio
