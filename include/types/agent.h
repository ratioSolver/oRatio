#pragma once

#include "smart_type.h"
#include "constructor.h"

#define AGENT_NAME "Agent"

namespace ratio::solver
{
  class agent final : public smart_type
  {
  public:
    agent(solver &slv);
    agent(const agent &orig) = delete;

  private:
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() override;

    void new_predicate(ratio::core::predicate &pred) noexcept override;
    void new_atom_flaw(atom_flaw &f) override;

    class agnt_constructor : public ratio::core::constructor
    {
    public:
      agnt_constructor(agent &agnt);
      agnt_constructor(agnt_constructor &&) = delete;
      virtual ~agnt_constructor();
    };

    class agnt_atom_listener : public atom_listener
    {
    public:
      agnt_atom_listener(agent &agnt, ratio::core::atom &a);
      agnt_atom_listener(agnt_atom_listener &&) = delete;
      virtual ~agnt_atom_listener();

    private:
      void something_changed();

      void sat_value_change(const semitone::var &) override { something_changed(); }
      void rdl_value_change(const semitone::var &) override { something_changed(); }
      void lra_value_change(const semitone::var &) override { something_changed(); }
      void ov_value_change(const semitone::var &) override { something_changed(); }

    protected:
      agent &agnt;
    };

  private:
    std::set<ratio::core::atom *> to_check;
    std::vector<std::pair<ratio::core::atom *, std::unique_ptr<agnt_atom_listener>>> atoms;
  };
} // namespace ratio::solver