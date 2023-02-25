#pragma once

#include "oratiosolver_export.h"
#include "lit.h"
#include "rational.h"
#include "json.h"
#include "memory.h"
#include <vector>
#include <functional>

namespace ratio
{
  class solver;
  class resolver;

  /**
   * @brief A class for representing flaws. Flaws are meta-variables that are used to represent the current state of the planning process. Flaws can be solver by applying resolvers.
   *
   */
  class flaw
  {
    friend class solver;
    friend class resolver;

  public:
    flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, const bool &exclusive = false);
    virtual ~flaw() = default;

    /**
     * @brief Get the solver this flaw belongs to.
     *
     * @return const solver& The solver.
     */
    solver &get_solver() noexcept { return s; }

    /**
     * @brief Get the resolvers that caused this flaw.
     *
     * @return std::vector<std::reference_wrapper<resolver>>& The resolvers that caused this flaw.
     */
    std::vector<std::reference_wrapper<resolver>> &get_causes() noexcept { return causes; }

    /**
     * @brief Gets the estimated cost of this flaw.
     *
     * @return const utils::rational& the estimated cost of this flaw.
     */
    utils::rational get_estimated_cost() const noexcept { return est_cost; }

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
     * @return const semitone::lit& The phi literal.
     */
    const semitone::lit &get_phi() const noexcept { return phi; }

    /**
     * @brief Get the position variable associated to this flaw.
     *
     * @return const semitone::var& The position variable.
     */
    const semitone::var &get_position() const noexcept { return position; }

    /**
     * @brief Get the resolvers for solving this flaw.
     *
     * @return std::vector<std::reference_wrapper<resolver>>& The resolvers.
     */
    std::vector<std::reference_wrapper<resolver>> &get_resolvers() noexcept { return resolvers; }

  private:
    /**
     * @brief Initialize the flaw.
     *
     * @pre the solver must be at root-level.
     */
    void init() noexcept;
    /**
     * Expands this flaw, invoking the compute_resolvers procedure.
     *
     * @pre the solver must be at root-level.
     */
    void expand();
    virtual void compute_resolvers() = 0;

  public:
    /**
     * @brief Gets a json representation of the data of this flaw.
     *
     * @return json::json the data of this flaw.
     */
    virtual json::json get_data() const noexcept = 0;

  protected:
    /**
     * Adds the resolver `r` to this flaw.
     */
    ORATIOSOLVER_EXPORT void add_resolver(utils::u_ptr<resolver> r);

  private:
    solver &s;                                                     // the solver this flaw belongs to..
    std::vector<std::reference_wrapper<resolver>> causes;          // the resolvers that caused this flaw..
    const bool exclusive;                                          // whether this flaw is exclusive or not..
    semitone::lit phi;                                             // the literal indicating whether the flaw is active or not (this literal is initialized by the `init` procedure)..
    utils::rational est_cost = utils::rational::POSITIVE_INFINITY; // the current estimated cost of the flaw..
    semitone::var position;                                        // the position variable (i.e., an integer time-point) associated to this flaw..
    bool expanded = false;                                         // whether this flaw has been expanded or not..
    std::vector<std::reference_wrapper<resolver>> resolvers;       // the resolvers for this flaw..
    std::vector<std::reference_wrapper<resolver>> supports;        // the resolvers supported by this flaw (used for propagating cost estimates)..
  };

  using flaw_ptr = utils::u_ptr<flaw>;

  /**
   * @brief Gets the id of the given flaw.
   *
   * @param f the flaw to get the id of.
   * @return uintptr_t the id of the given flaw.
   */
  inline uintptr_t get_id(const flaw &f) noexcept { return reinterpret_cast<uintptr_t>(&f); }
} // namespace ratio
