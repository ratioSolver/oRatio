#pragma once

#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>
#include "solver.hpp"
#include "flaw.hpp"
#include "resolver.hpp"

namespace ratio
{
  class smart_type;
  class graph : public semitone::theory
  {
    friend class smart_type;

  public:
    graph(solver &slv) noexcept;

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
        phis[variable(f->get_phi())].push_back(std::unique_ptr<flaw>(f));
        flaw_q.push_back(*f); // we add the flaw to the flaw queue..
      }
      else // we add the flaw to the pending flaws..
        pending_flaws.push_back(std::unique_ptr<flaw>(f));
      return *f;
    }

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
      rhos[variable(r->get_rho())].push_back(std::unique_ptr<resolver>(r));
      r->get_flaw().resolvers.push_back(*r);
      return *r;
    }

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

    void set_ni(const utils::lit &v) noexcept
    {
      tmp_ni = ni;
      ni = v;
    }

    void restore_ni() noexcept { ni = tmp_ni; }

  private:
    bool propagate(const utils::lit &) noexcept override { return true; }
    bool check() noexcept override { return true; }
    void push() noexcept override {}
    void pop() noexcept override {}

  private:
    solver &slv;                                                                    // the solver this graph belongs to..
    std::unordered_map<VARIABLE_TYPE, std::vector<std::unique_ptr<flaw>>> phis;     // the phi variables (propositional variable to flaws) of the flaws..
    std::vector<std::unique_ptr<flaw>> pending_flaws;                               // pending flaws, waiting for root-level to be initialized..
    std::unordered_map<VARIABLE_TYPE, std::vector<std::unique_ptr<resolver>>> rhos; // the rho variables (propositional variable to resolver) of the resolvers..
    std::optional<std::reference_wrapper<resolver>> res;                            // the current resolver..
    utils::lit tmp_ni;                                                              // the temporary controlling literal, used for restoring the controlling literal..
    utils::lit ni{utils::TRUE_lit};                                                 // the current controlling literal..
    std::deque<std::reference_wrapper<flaw>> flaw_q;                                // the flaw queue (for the graph building procedure)..
    std::unordered_set<flaw *> active_flaws;                                        // the currently active flaws..
    VARIABLE_TYPE gamma;                                                            // the variable representing the validity of this graph..
  };
} // namespace ratio