#pragma once

#include "oratiosolver_export.h"
#include "lit.h"
#include "rational.h"
#include <vector>
#include <memory>

namespace ratio::solver
{
  class solver;
  class resolver;

  class flaw
  {
    friend class solver;
    friend class resolver;

  public:
    /**
     * @brief Construct a new flaw object.
     *
     * @param slv the solver this flaw belongs to.
     * @param causes the resolvers that cause this flaw.
     * @param exclusive whether the flaw is exclusive or not.
     */
    ORATIOSOLVER_EXPORT flaw(solver &slv, std::vector<resolver *> causes, const bool &exclusive = false);
    flaw(const flaw &orig) = delete;
    virtual ~flaw() = default;

    inline solver &get_solver() const noexcept { return slv; }
    /**
     * @brief Gets the phi variable of this flaw.
     *
     * @return const semitone::lit& the phi variable of this flaw.
     */
    inline semitone::lit get_phi() const noexcept { return phi; }
    /**
     * @brief Gets the position variable of this flaw.
     *
     * @return const semitone::var& the position variable of this flaw.
     */
    inline semitone::var get_position() const noexcept { return position; }
    /**
     * @brief Gets the resolvers of this flaw.
     *
     * @return const std::vector<resolver *>& the resolvers of this flaw.
     */
    inline const std::vector<resolver *> &get_resolvers() const noexcept { return resolvers; }
    /**
     * @brief Gets the causes for having this flaw.
     *
     * @return const std::vector<resolver *>& the causes for having this flaw.
     */
    inline const std::vector<resolver *> &get_causes() const noexcept { return causes; }
    /**
     * @brief Gets the supports of this flaw.
     *
     * @return const std::vector<resolver *>& the supports of this flaw.
     */
    inline const std::vector<resolver *> &get_supports() const noexcept { return supports; }

    /**
     * @brief Gets the estimated cost of this flaw.
     *
     * @return const semitone::rational& the estimated cost of this flaw.
     */
    inline semitone::rational get_estimated_cost() const noexcept { return est_cost; }
    inline bool is_expanded() const noexcept { return expanded; }

    /**
     * @brief Gets the cheapest resolver of this flaw.
     *
     * @return resolver* the cheapest resolver of this flaw.
     */
    ORATIOSOLVER_EXPORT resolver *get_cheapest_resolver() const noexcept;
    /**
     * @brief Gets the best resolver of this flaw which, by default, corresponds to the cheapest resolver.
     *
     * @return resolver* the best resolver of this flaw.
     */
    virtual resolver *get_best_resolver() const noexcept { return get_cheapest_resolver(); }

    /**
     * @brief Gets a json representation of the data of this flaw.
     *
     * @return json::json the data of this flaw.
     */
    virtual json::json get_data() const noexcept = 0;

  private:
    /**
     * Initializes this flaw.
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
    void add_resolver(std::unique_ptr<resolver> r);

  private:
    solver &slv;                                                         // the solver this flaw belongs to..
    semitone::lit phi;                                                   // the propositional literal indicating whether the flaw is active or not (this literal is initialized by the `init` procedure)..
    semitone::var position;                                              // the position variable (i.e., an integer time-point) associated to this flaw..
    semitone::rational est_cost = semitone::rational::POSITIVE_INFINITY; // the current estimated cost of the flaw..
    bool expanded = false;                                               // a boolean indicating whether the flaw has been expanded..
    std::vector<resolver *> resolvers;                                   // the resolvers for this flaw..
    const std::vector<resolver *> causes;                                // the causes for having this flaw (used for activating the flaw through causal propagation)..
    std::vector<resolver *> supports;                                    // the resolvers supported by this flaw (used for propagating cost estimates)..
    const bool exclusive;                                                // a boolean indicating whether the flaw is exclusive (i.e. exactly one of its resolver can be applied)..
  };

  /**
   * @brief Gets the id of the given flaw.
   *
   * @param f the flaw to get the id of.
   * @return uintptr_t the id of the given flaw.
   */
  inline uintptr_t get_id(const flaw &f) noexcept { return reinterpret_cast<uintptr_t>(&f); }
  ORATIOSOLVER_EXPORT json::json to_json(const flaw &rhs) noexcept;
} // namespace ratio::solver
