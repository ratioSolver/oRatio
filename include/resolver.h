#pragma once

#include "lit.h"
#include "rational.h"
#include "flaw.h"
#include <vector>
#include <functional>

namespace ratio
{
  class solver;
  class flaw;

  /**
   * @brief A class for representing resolvers. Resolvers are meta-values that are used to represent the current state of the planning process. Resolvers can be applied to solve flaws.
   *
   */
  class resolver
  {
    friend class solver;
    friend class flaw;

  public:
    resolver(flaw &f, const utils::rational &cost);
    resolver(flaw &f, const semitone::lit &rho, const utils::rational &cost);
    virtual ~resolver() = default;

    /**
     * @brief Get the solver this resolver belongs to.
     *
     * @return const solver& The solver.
     */
    solver &get_solver() noexcept { return f.s; }

    /**
     * @brief Get the solver this resolver belongs to.
     *
     * @return const solver& The solver.
     */
    const solver &get_solver() const noexcept { return f.s; }

    /**
     * @brief Get the flaw this resolver solves.
     *
     * @return flaw& The flaw.
     */
    flaw &get_flaw() noexcept { return f; }

    /**
     * @brief Get the flaw this resolver solves.
     *
     * @return const flaw& The flaw.
     */
    const flaw &get_flaw() const noexcept { return f; }

    /**
     * @brief Gets the rho variable of this resolver.
     *
     * @return const semitone::lit& the rho variable of this resolver.
     */
    semitone::lit get_rho() const noexcept { return rho; }

    /**
     * @brief Gets the intrinsic cost of this resolver.
     *
     * @return const semitone::rational& the intrinsic cost of this resolver.
     */
    const utils::rational &get_intrinsic_cost() const noexcept { return intrinsic_cost; }
    /**
     * @brief Gets the estimated cost of this resolver.
     *
     * @return const utils::rational& the estimated cost of this resolver.
     */
    utils::rational get_estimated_cost() const noexcept;

    /**
     * @brief Gets the preconditions of this resolver.
     *
     * @return const std::vector<std::reference_wrapper<flaw>>& the preconditions of this resolver.
     */
    const std::vector<std::reference_wrapper<flaw>> &get_preconditions() const noexcept { return preconditions; }

    /**
     * Applies this resolver, introducing subgoals and/or constraints.
     *
     * @pre the solver must be at root-level.
     */
    virtual void apply() = 0;

    /**
     * @brief Gets a json representation of the data of this resolver.
     *
     * @return const json::json& the data of this resolver.
     */
    virtual json::json get_data() const noexcept = 0;

    friend std::string to_string(const resolver &r) noexcept;

    friend json::json to_json(const resolver &r) noexcept;

  private:
    flaw &f;                                                 // the flaw solved by this resolver..
    const semitone::lit rho;                                 // the propositional literal indicating whether the resolver is active or not..
    const utils::rational intrinsic_cost;                    // the intrinsic cost of the resolver..
    std::vector<std::reference_wrapper<flaw>> preconditions; // the preconditions of this resolver..
  };

  using resolver_ptr = utils::u_ptr<resolver>;

  /**
   * @brief Gets the id of the given resolver.
   *
   * @param r the resolver to get the id of.
   * @return uintptr_t the id of the given resolver.
   */
  inline uintptr_t get_id(const resolver &r) noexcept { return reinterpret_cast<uintptr_t>(&r); }
} // namespace ratio
