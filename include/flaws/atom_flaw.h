#pragma once

#include "flaw.h"
#include "resolver.h"
#include "item.h"

namespace ratio::solver
{
  class smart_type;

  bool is_unification(const resolver &r);

  class atom_flaw final : public flaw
  {
    friend class smart_type;

  public:
    atom_flaw(solver &slv, std::vector<resolver *> causes, ratio::core::atom &a, const bool is_fact);
    atom_flaw(const atom_flaw &orig) = delete;

    inline ratio::core::atom &get_atom() const noexcept { return atm; }

  private:
    void compute_resolvers() override;

  private:
    class activate_fact final : public resolver
    {
    public:
      activate_fact(atom_flaw &f, ratio::core::atom &a);
      activate_fact(const semitone::lit &r, atom_flaw &f, ratio::core::atom &a);
      activate_fact(const activate_fact &that) = delete;

    private:
      void apply() override;

    private:
      ratio::core::atom &atm; // the fact which will be activated..
    };

    class activate_goal final : public resolver
    {
    public:
      activate_goal(atom_flaw &f, ratio::core::atom &a);
      activate_goal(const semitone::lit &r, atom_flaw &f, ratio::core::atom &a);
      activate_goal(const activate_goal &that) = delete;

    private:
      void apply() override;

    private:
      ratio::core::atom &atm; // the goal which will be activated..
    };

    class unify_atom final : public resolver
    {
    public:
      unify_atom(atom_flaw &atm_flaw, ratio::core::atom &atm, ratio::core::atom &trgt, const std::vector<semitone::lit> &unif_lits);
      unify_atom(const unify_atom &that) = delete;

    private:
      void apply() override;

    private:
      ratio::core::atom &atm;                     // the unifying atom..
      ratio::core::atom &trgt;                    // the target atom..
      const std::vector<semitone::lit> unif_lits; // the unification literals..
    };

    friend inline bool is_unification(const resolver &r) { return dynamic_cast<const unify_atom *>(&r); }

  private:
    ratio::core::atom &atm; // the atom which has to be justified..

  public:
    const bool is_fact;
  };
} // namespace ratio::solver