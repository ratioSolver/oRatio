#pragma once

#include "flaw.hpp"
#include "solver.hpp"

namespace ratio
{
  class atom_flaw : public flaw
  {
  public:
    atom_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>> &&arguments) noexcept;

    [[nodiscard]] std::shared_ptr<atom> get_atom() const noexcept { return atm; }

  private:
    std::shared_ptr<atom> atm;
  };
} // namespace ratio
