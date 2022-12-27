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
    /**
     * @brief Construct a new resolver object.
     *
     * @param slv the solver this resolver belongs to.
     * @param cost the intrinsic cost of this resolver.
     * @param eff the flaw this resolver resolves.
     */
    ORATIOSOLVER_EXPORT resolver(semitone::rational cost, flaw &eff);
    /**
     * @brief Construct a new resolver object.
     *
     * @param slv the solver this resolver belongs to.
     * @param r the propositional literal indicating whether this resolver is active or not.
     * @param cost the intrinsic cost of this resolver.
     * @param eff the flaw this resolver resolves.
     */
    ORATIOSOLVER_EXPORT resolver(semitone::lit r, semitone::rational cost, flaw &eff);
    resolver(const resolver &that) = delete;
    virtual ~resolver() = default;

    inline solver &get_solver() const noexcept { return slv; }
    /**
     * @brief Gets the rho variable of this resolver.
     *
     * @return const semitone::lit& the rho variable of this resolver.
     */
    inline semitone::lit get_rho() const noexcept { return rho; }
    /**
     * @brief Gets the intrinsic cost of this resolver.
     *
     * @return const semitone::rational& the intrinsic cost of this resolver.
     */
    inline semitone::rational get_intrinsic_cost() const noexcept { return intrinsic_cost; }
    /**
     * @brief Gets the estimated cost of this resolver.
     *
     * @return const semitone::rational& the estimated cost of this resolver.
     */
    ORATIOSOLVER_EXPORT semitone::rational get_estimated_cost() const noexcept;
    /**
     * @brief Gets the flaw this resolver resolves.
     *
     * @return const flaw& the flaw this resolver resolves.
     */
    inline flaw &get_effect() const noexcept { return effect; }
    /**
     * @brief Gets the preconditions of this resolver.
     *
     * @return const std::vector<flaw *>& the preconditions of this resolver.
     */
    inline const std::vector<flaw *> &get_preconditions() const noexcept { return preconditions; }

    /**
     * @brief Gets a json representation of the data of this resolver.
     *
     * @return const json::json& the data of this resolver.
     */
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

  /**
   * @brief Gets the id of the given resolver.
   *
   * @param r the resolver to get the id of.
   * @return uintptr_t the id of the given resolver.
   */
  inline uintptr_t get_id(const resolver &r) noexcept { return reinterpret_cast<uintptr_t>(&r); }
  ORATIOSOLVER_EXPORT json::json to_json(const resolver &rhs) noexcept;
} // namespace ratio::solver
