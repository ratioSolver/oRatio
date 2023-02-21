#pragma once

#include "oratiosolver_export.h"
#include "lit.h"
#include "rational.h"
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
  public:
    flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, const bool &exclusive = false);
    virtual ~flaw() = default;

    /**
     * @brief Get the solver this flaw belongs to.
     *
     * @return const solver& The solver.
     */
    const solver &get_solver() const noexcept { return s; }

    /**
     * @brief Get the phi literal indicating whether the flaw is active or not.
     *
     * @return const semitone::lit& The phi literal.
     */
    const semitone::lit &get_phi() const noexcept { return phi; }

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
    virtual void compute_resolvers() {}

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
