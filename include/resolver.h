#pragma once

#include "oratio_export.h"
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
    ORATIO_EXPORT resolver(semitone::rational cost, flaw &eff);
    ORATIO_EXPORT resolver(semitone::lit r, semitone::rational cost, flaw &eff);
    resolver(const resolver &that) = delete;
    virtual ~resolver() = default;

    inline solver &get_solver() const noexcept { return slv; }
    inline semitone::lit get_rho() const noexcept { return rho; }
    inline semitone::rational get_intrinsic_cost() const noexcept { return intrinsic_cost; }
    ORATIO_EXPORT semitone::rational get_estimated_cost() const noexcept;
    inline flaw &get_effect() const noexcept { return effect; }
    inline const std::vector<flaw *> &get_preconditions() const noexcept { return preconditions; }

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
} // namespace ratio::solver
