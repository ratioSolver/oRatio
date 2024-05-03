#pragma once

#include <cstdint>
#include <vector>
#include "rational.hpp"
#include "lit.hpp"

#ifdef ENABLE_VISUALIZATION
#include "json.hpp"
#include "bool.hpp"
#endif

namespace ratio
{
  class solver;
  class graph;
  class flaw;

  /**
   * @class resolver
   * @brief Represents a resolver for solving a flaw in a graph.
   *
   * The `resolver` class provides functionality to solve a specific flaw in a graph by introducing subgoals and/or constraints.
   * It contains information about the flaw it solves, the rho variable, the intrinsic cost, and the preconditions.
   * Derived classes must implement the `apply()` function to apply the resolver.
   */
  class resolver
  {
    friend class graph;
    friend class flaw;

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
    [[nodiscard]] utils::rational get_estimated_cost() const noexcept;
    /**
     * @brief Get the preconditions of this resolver.
     *
     * @return std::vector<std::reference_wrapper<flaw>>& The preconditions of this resolver.
     */
    [[nodiscard]] const std::vector<std::reference_wrapper<flaw>> &get_preconditions() const noexcept { return preconditions; }

    /**
     * Applies this resolver, introducing subgoals and/or constraints.
     *
     * @pre the solver must be at root-level.
     */
    virtual void apply() = 0;

#ifdef ENABLE_VISUALIZATION
    /**
     * @brief Get a JSON representation of the data of the resolver.
     *
     * @return json::json the data of the resolver.
     */
    virtual json::json get_data() const noexcept = 0;

    friend json::json to_json(const resolver &f) noexcept;
#endif

  private:
    flaw &f;                                                 // the flaw solved by this resolver..
    const utils::lit rho;                                    // the propositional literal indicating whether the resolver is active or not..
    const utils::rational intrinsic_cost;                    // the intrinsic cost of the resolver..
    std::vector<std::reference_wrapper<flaw>> preconditions; // the preconditions of this resolver..
  };

#ifdef ENABLE_VISUALIZATION
  json::json to_json(const resolver &r) noexcept;

  std::string to_state(const resolver &r) noexcept;
#endif

  /**
   * @brief Gets the unique identifier of the given resolver.
   *
   * @param r the resolver to get the unique identifier of.
   * @return uintptr_t the unique identifier of the given resolver.
   */
  inline uintptr_t get_id(const resolver &r) noexcept { return reinterpret_cast<uintptr_t>(&r); }
} // namespace ratio
