#pragma once

#include "smart_type.h"
#include "constructor.h"

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
      agnt_constructor(agent &agnt) : riddle::constructor(agnt, {}, {}) {}
    };

    class agnt_atom_listener final : public atom_listener
    {
    public:
      agnt_atom_listener(agent &agnt, atom &a) : atom_listener(a), agnt(agnt) {}

    private:
      void something_changed() { agnt.to_check.insert(&atm); }

      void sat_value_change(const semitone::var &) override { something_changed(); }
      void rdl_value_change(const semitone::var &) override { something_changed(); }
      void lra_value_change(const semitone::var &) override { something_changed(); }
      void ov_value_change(const semitone::var &) override { something_changed(); }

    protected:
      agent &agnt;
    };

    using agnt_atom_listener_ptr = utils::u_ptr<agnt_atom_listener>;

    json::json extract() const noexcept override;

  private:
    std::set<atom *> to_check;
    std::vector<atom *> atoms;
    std::vector<agnt_atom_listener_ptr> listeners;
  };
} // namespace ratio
