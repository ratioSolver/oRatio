#pragma once

#include "oratiosolver_export.h"
#include "lit.h"
#include "rational.h"
#include <vector>

namespace ratio::solver
{
  class solver;
  class flaw;

  class resolver
  {
    friend class solver;
    friend class flaw;

  public:
    ORATIOSOLVER_EXPORT resolver(semitone::rational cost, flaw &eff);
    ORATIOSOLVER_EXPORT resolver(semitone::lit r, semitone::rational cost, flaw &eff);
    resolver(const resolver &that) = delete;
    virtual ~resolver() = default;

    inline solver &get_solver() const noexcept { return slv; }
    inline semitone::lit get_rho() const noexcept { return rho; }
    inline semitone::rational get_intrinsic_cost() const noexcept { return intrinsic_cost; }
    ORATIOSOLVER_EXPORT semitone::rational get_estimated_cost() const noexcept;
    inline flaw &get_effect() const noexcept { return effect; }
    inline const std::vector<flaw *> &get_preconditions() const noexcept { return preconditions; }

    virtual json::json get_data() const noexcept = 0;

  private:
    /**
     * Applies this resolver, introducing subgoals and/or constraints.
     *
     * @pre the solver must be at root-level.
     */
    virtual void apply() = 0;

  private:
    solver &slv;                             // the solver this resolver belongs to..
    const semitone::lit rho;                 // the propositional literal indicating whether the resolver is active or not..
    const semitone::rational intrinsic_cost; // the intrinsic cost of the resolver..
    flaw &effect;                            // the flaw solved by this resolver..
    std::vector<flaw *> preconditions;       // the preconditions of this resolver..
  };

  inline uintptr_t get_id(const resolver &r) noexcept { return reinterpret_cast<uintptr_t>(&r); }
  ORATIOSOLVER_EXPORT json::json to_json(const resolver &rhs) noexcept;
} // namespace ratio::solver
