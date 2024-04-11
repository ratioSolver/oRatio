#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <cstdint>
#include "lit.hpp"
#include "rational.hpp"

namespace ratio
{
  class solver;
  class graph;
  class resolver;

  class flaw
  {
    friend class graph;
    friend class resolver;

  public:
    flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, const bool &exclusive = false) noexcept;
    virtual ~flaw() = default;

    /**
     * @brief Get the solver this flaw belongs to.
     *
     * @return solver& The solver this flaw belongs to.
     */
    [[nodiscard]] solver &get_solver() const noexcept { return s; }

    /**
     * @brief Get the resolvers that caused this flaw.
     *
     * @return std::vector<std::reference_wrapper<resolver>>& The resolvers that caused this flaw.
     */
    std::vector<std::reference_wrapper<resolver>> &get_causes() noexcept { return causes; }

    /**
     * @brief Get the resolvers that caused this flaw.
     *
     * @return const std::vector<std::reference_wrapper<resolver>>& The resolvers that caused this flaw.
     */
    const std::vector<std::reference_wrapper<resolver>> &get_causes() const noexcept { return causes; }

    /**
     * @brief Get the resolvers that supported by this flaw.
     *
     * @return std::vector<std::reference_wrapper<resolver>>& The resolvers that supported by this flaw.
     */
    std::vector<std::reference_wrapper<resolver>> &get_supports() noexcept { return supports; }

    /**
     * @brief Gets the estimated cost of this flaw.
     *
     * @return const utils::rational& the estimated cost of this flaw.
     */
    utils::rational get_estimated_cost() const noexcept { return est_cost; }

    /**
     * @brief Get the position variable associated to this flaw.
     *
     * @return VARIABLE_TYPE the position variable associated to this flaw.
     */
    VARIABLE_TYPE get_position() const noexcept { return position; }

    /**
     * @brief Check whether this flaw has been expanded.
     *
     * @return true If the flaw has been expanded.
     * @return false If the flaw has not been expanded.
     */
    bool is_expanded() const noexcept { return expanded; }

    /**
     * @brief Get the phi literal indicating whether the flaw is active or not.
     *
     * @return const utils::lit& The phi literal.
     */
    const utils::lit &get_phi() const noexcept { return phi; }

    /**
     * @brief Get the resolvers for solving this flaw.
     *
     * @return std::vector<std::reference_wrapper<resolver>>& The resolvers.
     */
    std::vector<std::reference_wrapper<resolver>> &get_resolvers() noexcept { return resolvers; }

    /**
     * @brief Gets the cheapest resolver of this flaw.
     *
     * @return resolver& the cheapest resolver of this flaw.
     */
    resolver &get_cheapest_resolver() const noexcept;
    /**
     * @brief Gets the best resolver of this flaw which, by default, corresponds to the cheapest resolver.
     *
     * @return resolver& the best resolver of this flaw.
     */
    virtual resolver &get_best_resolver() const noexcept { return get_cheapest_resolver(); }

  private:
    /**
     * @brief Initialize the flaw.
     *
     * @pre the solver must be at root-level.
     */
    void init() noexcept;
    /**
     * Computes the resolvers for this flaw.
     */
    virtual void compute_resolvers() = 0;

  private:
    solver &s;                                                     // the solver this flaw belongs to..
    std::vector<std::reference_wrapper<resolver>> causes;          // the resolvers that caused this flaw..
    bool exclusive;                                                // whether this flaw is exclusive..
    utils::lit phi;                                                // the literal indicating whether the flaw is active or not (this literal is initialized by the `init` procedure)..
    utils::rational est_cost = utils::rational::positive_infinite; // the current estimated cost of the flaw..
    VARIABLE_TYPE position;                                        // the position variable (i.e., an integer time-point) associated to this flaw..
    bool expanded = false;                                         // whether this flaw has been expanded or not..
    std::vector<std::reference_wrapper<resolver>> resolvers;       // the resolvers for this flaw..
    std::vector<std::reference_wrapper<resolver>> supports;        // the resolvers supported by this flaw (used for propagating cost estimates)..
  };

  /**
   * @brief Gets the id of the given flaw.
   *
   * @param f the flaw to get the id of.
   * @return uintptr_t the id of the given flaw.
   */
  inline uintptr_t get_id(const flaw &f) noexcept { return reinterpret_cast<uintptr_t>(&f); }
} // namespace ratio
