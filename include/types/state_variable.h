#pragma once

#include "smart_type.h"

#define STATE_VARIABLE_NAME "StateVariable"

namespace ratio
{
  class state_variable final : public smart_type
  {
  public:
    state_variable(riddle::scope &scp);

  private:
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() override;

    void new_predicate(riddle::predicate &pred) noexcept override;
    void new_atom(atom &atm) override;
  };
} // namespace ratio
