#pragma once

#include "smart_type.h"

#define AGENT_NAME "Agent"

namespace ratio
{
  class agent final : public smart_type, public timeline
  {
  public:
    agent(riddle::scope &scp);

    const std::vector<atom *> &get_atoms() const noexcept { return atoms; }

  private:
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() override { return {}; }

    void new_atom(atom &atm) override;

    class agnt_constructor final : public riddle::constructor
    {
    public:
      agnt_constructor(agent &agnt) : riddle::constructor(agnt, {}, {}, {}, {}) {}
    };

    json::json extract() const noexcept override;

  private:
    std::vector<atom *> atoms;
  };
} // namespace ratio
