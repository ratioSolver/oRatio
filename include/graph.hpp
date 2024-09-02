#pragma once

#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>
#include "solver.hpp"
#include "flaw.hpp"
#include "resolver.hpp"

#ifdef ENABLE_API
#define NEW_FLAW(f) slv.flaw_created(f)
#define FLAW_STATE_CHANGED(f) slv.flaw_state_changed(f)
#define FLAW_COST_CHANGED(f) slv.flaw_cost_changed(f)
#define FLAW_POSITION_CHANGED(f) slv.flaw_position_changed(f)
#define NEW_RESOLVER(r) slv.resolver_created(r)
#define RESOLVER_STATE_CHANGED(r) slv.resolver_state_changed(r)
#define NEW_CAUSAL_LINK(f, r) slv.causal_link_added(f, r)
#else
#define NEW_FLAW(f)
#define FLAW_STATE_CHANGED(f)
#define FLAW_COST_CHANGED(f)
#define FLAW_POSITION_CHANGED(f)
#define NEW_RESOLVER(r)
#define RESOLVER_STATE_CHANGED(r)
#define NEW_CAUSAL_LINK(f, r)
#endif

namespace ratio
{
  class smart_type;

  /**
   * @brief Defines the causal graph for representing flaws and resolvers.
   *
   * The graph class is responsible for managing the flaws and resolvers in the system.
   * It provides functions to create new flaws and resolvers, establish causal links between them,
   * and retrieve information about the flaws and resolvers in the graph.
   */
  class graph : public semitone::theory
  {
    friend class smart_type;
    friend class solver;

  public:
    graph(solver &slv) noexcept;

    /**
     * @brief Returns a reference to the solver object.
     *
     * This function returns a reference to the solver object associated with the graph.
     *
     * @return A reference to the solver object.
     */
    [[nodiscard]] solver &get_solver() const noexcept { return slv; }

    /**
     * @brief Creates a new flaw of the given type.
     *
     * @tparam Tp The type of the flaw to create.
     * @tparam Args The types of the arguments to pass to the flaw
     * @param args The arguments to pass to the flaw
     * @return Tp& The created flaw
     */
    template <typename Tp, typename... Args>
    Tp &new_flaw(Args &&...args) noexcept
    {
      static_assert(std::is_base_of_v<flaw, Tp>, "Tp must be a subclass of flaw");
      auto f = new Tp(std::forward<Args>(args)...);
      if (slv.get_sat().root_level())
      { // if we are at root-level, we initialize the flaw and add it to the flaw queue for future expansion..
        f->init();
        NEW_FLAW(*f);
        phis[variable(f->phi)].push_back(std::unique_ptr<flaw>(f));
        flaw_q.push_back(*f); // we add the flaw to the flaw queue..

        if (get_sat().value(f->phi) == utils::True)
          active_flaws.insert(f); // the flaw is already active..
        else
          bind(variable(f->phi)); // we listen for the flaw to become active..
      }
      else // we add the flaw to the pending flaws..
        pending_flaws.push_back(std::unique_ptr<flaw>(f));
      return *f;
    }

    /**
     * Returns a vector of flaws in the graph.
     *
     * @return A vector of reference wrappers to the flaws.
     */
    std::vector<std::reference_wrapper<flaw>> get_flaws() const noexcept
    {
      std::vector<std::reference_wrapper<flaw>> res;
      for (auto &f : phis)
        for (auto &f_ : f.second)
          res.push_back(*f_);
      return res;
    }

    /**
     * @brief Get the current flaw.
     *
     * This function returns an optional reference to the current flaw.
     *
     * @return An optional reference to the current flaw.
     */
    const std::optional<std::reference_wrapper<flaw>> &get_current_flaw() const noexcept { return c_flaw; }

    /**
     * @brief Creates a new resolver of the given type.
     *
     * @tparam Tp The type of the resolver to create.
     * @tparam Args The types of the arguments to pass to the resolver
     * @param args The arguments to pass to the resolver
     * @return Tp& The created resolver
     */
    template <typename Tp, typename... Args>
    Tp &new_resolver(Args &&...args) noexcept
    {
      static_assert(std::is_base_of_v<resolver, Tp>, "Tp must be a subclass of resolver");
      auto r = new Tp(std::forward<Args>(args)...);
      NEW_RESOLVER(*r);
      rhos[variable(r->get_rho())].push_back(std::unique_ptr<resolver>(r));
      r->get_flaw().resolvers.push_back(*r);

      switch (get_sat().value(r->rho))
      {
      case utils::True: // we have a top-level (a landmark) resolver..
        active_flaws.erase(&r->get_flaw());
        break;
      case utils::Undefined:    // we do not have a top-level (a landmark) resolver, nor an infeasible one..
        bind(variable(r->rho)); // we listen for the resolver to become active..
        break;
      }
      return *r;
    }

    /**
     * Returns a vector of resolvers in the graph.
     *
     * @return A vector of reference wrappers to the resolvers.
     */
    std::vector<std::reference_wrapper<resolver>> get_resolvers() const noexcept
    {
      std::vector<std::reference_wrapper<resolver>> res;
      for (auto &r : rhos)
        for (auto &r_ : r.second)
          res.push_back(*r_);
      return res;
    }

    /**
     * Checks if the graph has any active flaws.
     *
     * @return true if there are active flaws, false otherwise.
     */
    bool has_active_flaws() const noexcept { return !active_flaws.empty(); }

    /**
     * @brief Checks if the graph has any flaws with infinite cost.
     *
     * If the graph has flaws with infinite cost, it is impossible to resolve them.
     *
     * @return true if the graph has flaws with infinite cost, false otherwise.
     */
    bool has_infinite_cost_active_flaws() const noexcept;

    /**
     * @brief Returns the most expensive flaw.
     *
     * This function returns a reference to the most expensive flaw in the graph. The most expensive flaw is the best candidate to be resolved next.
     *
     * @return A reference to the most expensive flaw.
     */
    flaw &get_most_expensive_flaw() const noexcept;

    /**
     * @brief Get the current resolver.
     *
     * This function returns an optional reference to the current resolver.
     *
     * @return An optional reference to the current resolver.
     */
    const std::optional<std::reference_wrapper<resolver>> &get_current_resolver() const noexcept { return c_res; }

    /**
     * @brief Creates a new causal link between a flaw and a resolver.
     *
     * This function establishes a causal link between the given flaw and resolver.
     * A causal link represents a dependency relationship between a flaw and a resolver,
     * indicating that the flaw must be active for the resolver to be applicable.
     *
     * @param f The flaw to establish a causal link with.
     * @param r The resolver to establish a causal link with.
     */
    void new_causal_link(flaw &f, resolver &r);

    /**
     * @brief Gets the current controlling literal.
     *
     * If there is a current resolver, the controlling literal is the rho variable of the resolver. Otherwise, it is TRUE.
     *
     * @return const utils::lit& The current controlling literal.
     */
    const utils::lit &get_ni() const noexcept { return ni; }

    /**
     * @brief Builds the graph.
     *
     * This function is responsible for building the graph.
     */
    void build();

    /**
     * @brief Adds a new layer to the graph.
     *
     * This function is responsible for adding a new layer to the graph.
     */
    void add_layer();

    /**
     * @brief Computes the cost of the given flaw.
     *
     * The cost of a flaw is the cost of the best resolver that can solve it.
     * If the cost of the flaw changes, the function stores the old cost for backtracking and propagates the cost to the graph.
     *
     * @param f The flaw for which the cost is to be computed.
     */
    void compute_flaw_cost(flaw &f) noexcept;

  protected:
    /**
     * @brief Enqueues the given flaw in the graph.
     *
     * @param f The flaw to enqueue.
     */
    void enqueue(flaw &f) noexcept { flaw_q.push_back(f); }

    /**
     * @brief Expands the given flaw in the graph.
     *
     * @param f The flaw to expand.
     */
    void expand_flaw(flaw &f);

    /**
     * @brief Sets the value of `ni`.
     *
     * This function sets the value of `ni` to the specified `v`.
     *
     * @param v The new value for `ni`.
     */
    void set_ni(const utils::lit &v) noexcept
    {
      tmp_ni = ni;
      ni = v;
    }

    /**
     * @brief Restores the value of `ni` to its original value.
     *
     * This function restores the value of the `ni` variable to its original value
     * before any modifications were made.
     */
    void restore_ni() noexcept { ni = tmp_ni; }

  private:
    bool propagate(const utils::lit &p) noexcept override;
    bool check() noexcept override;
    void push() noexcept override;
    void pop() noexcept override;

  private:
    solver &slv;                                                                    // the solver this graph belongs to..
    std::unordered_map<VARIABLE_TYPE, std::vector<std::unique_ptr<flaw>>> phis;     // the phi variables (propositional variable to flaws) of the flaws..
    std::vector<std::unique_ptr<flaw>> pending_flaws;                               // pending flaws, waiting for root-level to be initialized..
    std::unordered_map<VARIABLE_TYPE, std::vector<std::unique_ptr<resolver>>> rhos; // the rho variables (propositional variable to resolver) of the resolvers..
    std::optional<std::reference_wrapper<flaw>> c_flaw;                             // the current flaw..
    std::optional<std::reference_wrapper<resolver>> c_res;                          // the current resolver..
    utils::lit tmp_ni;                                                              // the temporary controlling literal, used for restoring the controlling literal..
    utils::lit ni{utils::TRUE_lit};                                                 // the current controlling literal..
    std::deque<std::reference_wrapper<flaw>> flaw_q;                                // the flaw queue (for the graph building procedure)..
    std::unordered_set<flaw *> visited;                                             // the visited flaws, for graph cost propagation (and deferrable flaws check)..
    std::unordered_set<flaw *> active_flaws;                                        // the currently active flaws..
    VARIABLE_TYPE gamma;                                                            // the variable representing the validity of this graph..

    /**
     * @brief Represents a layer in the solver.
     *
     * A layer contains information about the old estimated costs of flaws, the newly activated flaws, and the solved flaws.
     */
    struct layer
    {
      std::unordered_map<flaw *, utils::rational> old_f_costs; // the old estimated flaws` costs..
      std::unordered_set<flaw *> new_flaws;                    // the just activated flaws..
      std::unordered_set<flaw *> solved_flaws;                 // the just solved flaws..
    };
    std::vector<layer> trail; // the list of taken decisions, with the associated changes made, in chronological order..
  };
} // namespace ratio