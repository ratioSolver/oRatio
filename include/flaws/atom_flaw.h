#pragma once

#include "flaw.h"
#include "resolver.h"
#include "item.h"

namespace ratio
{
  class smart_type;

  class atom_flaw final : public flaw
  {
    friend class smart_type;

  public:
    atom_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, riddle::expr &atom);

  private:
    void compute_resolvers() override;

    json::json get_data() const noexcept override;

    riddle::expr &get_atom() noexcept { return atm; }
    const riddle::expr &get_atom() const noexcept { return atm; }

    class activate_fact final : public resolver
    {
    public:
      activate_fact(atom_flaw &ef);
      activate_fact(atom_flaw &ef, const semitone::lit &l);

      void apply() override;

      json::json get_data() const noexcept override;
    };

    class activate_goal final : public resolver
    {
    public:
      activate_goal(atom_flaw &ef);
      activate_goal(atom_flaw &ef, const semitone::lit &l);

      void apply() override;

      json::json get_data() const noexcept override;
    };

    class unify_atom final : public resolver
    {
    public:
      unify_atom(atom_flaw &ef, riddle::expr target, const semitone::lit &unif_lit);

      void apply() override;

      json::json get_data() const noexcept override;

    private:
      riddle::expr target;
      semitone::lit unif_lit;
    };

  private:
    riddle::expr &atm;
  };
} // namespace ratio
