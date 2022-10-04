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
    atom_flaw(solver &slv, std::vector<resolver *> causes, ratio::core::expr &a, const bool is_fact);
    atom_flaw(const atom_flaw &orig) = delete;

    inline ratio::core::atom &get_atom() const noexcept { return static_cast<ratio::core::atom &>(*atm); }
    inline ratio::core::expr get_atom_expr() const noexcept { return atm; }

    ORATIOSOLVER_EXPORT json::json get_data() const noexcept override;

  private:
    void compute_resolvers() override;

  private:
    class activate_fact final : public resolver
    {
    public:
      activate_fact(atom_flaw &f);
      activate_fact(const semitone::lit &r, atom_flaw &f);
      activate_fact(const activate_fact &that) = delete;

      ORATIOSOLVER_EXPORT json::json get_data() const noexcept override;

    private:
      void apply() override;
    };

    class activate_goal final : public resolver
    {
    public:
      activate_goal(atom_flaw &f);
      activate_goal(const semitone::lit &r, atom_flaw &f);
      activate_goal(const activate_goal &that) = delete;

      ORATIOSOLVER_EXPORT json::json get_data() const noexcept override;

    private:
      void apply() override;
    };

    class unify_atom final : public resolver
    {
    public:
      unify_atom(atom_flaw &atm_flaw, ratio::core::atom &trgt, const std::vector<semitone::lit> &unif_lits);
      unify_atom(const unify_atom &that) = delete;

      ORATIOSOLVER_EXPORT json::json get_data() const noexcept override;

    private:
      void apply() override;

    private:
      ratio::core::atom &trgt;                    // the target atom..
      const std::vector<semitone::lit> unif_lits; // the unification literals..
    };

    friend inline bool is_unification(const resolver &r) { return dynamic_cast<const unify_atom *>(&r); }

  private:
    ratio::core::expr atm; // the atom which has to be justified..

  public:
    const bool is_fact;
  };
} // namespace ratio::solver