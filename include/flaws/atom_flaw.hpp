#pragma once

#include "flaw.hpp"
#include "type.hpp"

namespace ratio
{
  class atom;

  class atom_flaw : public flaw
  {
  public:
    atom_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>> &&arguments) noexcept;

    [[nodiscard]] const std::shared_ptr<atom> &get_atom() const noexcept { return atm; }

  private:
    void compute_resolvers() override;

  private:
    std::shared_ptr<atom> atm;
  };
} // namespace ratio
