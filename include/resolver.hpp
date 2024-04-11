#pragma once

#include <cstdint>
#include <vector>
#include "rational.hpp"
#include "lit.hpp"

namespace ratio
{
  class solver;
  class flaw;

  class resolver
  {
  public:
    resolver(flaw &f, const utils::rational &intrinsic_cost);
    resolver(flaw &f, const utils::lit &rho, const utils::rational &intrinsic_cost);
    virtual ~resolver() = default;

    /**
     * @brief Get the flaw this resolver solves.
     *
     * @return flaw& The flaw.
     */
    [[nodiscard]] flaw &get_flaw() const noexcept { return f; }
    /**
     * @brief Gets the rho variable of this resolver.
     *
     * @return const semitone::lit& the rho variable of this resolver.
     */
    [[nodiscard]] const utils::lit &get_rho() const noexcept { return rho; }
    /**
     * @brief Gets the intrinsic cost of this resolver.
     *
     * @return const semitone::rational& the intrinsic cost of this resolver.
     */
    [[nodiscard]] const utils::rational &get_intrinsic_cost() const noexcept { return intrinsic_cost; }
    /**
     * @brief Gets the estimated cost of this resolver.
     *
     * @return const utils::rational& the estimated cost of this resolver.
     */
    utils::rational get_estimated_cost() const noexcept;

    /**
     * Applies this resolver, introducing subgoals and/or constraints.
     *
     * @pre the solver must be at root-level.
     */
    virtual void apply() = 0;

  private:
    flaw &f;                                                 // the flaw solved by this resolver..
    const utils::lit rho;                                    // the propositional literal indicating whether the resolver is active or not..
    const utils::rational intrinsic_cost;                    // the intrinsic cost of the resolver..
    std::vector<std::reference_wrapper<flaw>> preconditions; // the preconditions of this resolver..
  };

  /**
   * @brief Gets the id of the given resolver.
   *
   * @param r the resolver to get the id of.
   * @return uintptr_t the id of the given resolver.
   */
  inline uintptr_t get_id(const resolver &r) noexcept { return reinterpret_cast<uintptr_t>(&r); }
} // namespace ratio
