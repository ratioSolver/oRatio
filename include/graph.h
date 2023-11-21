#pragma once

#include "flaw.h"
#include "resolver.h"
#include <unordered_set>
#include <unordered_map>

#ifdef BUILD_LISTENERS
#define FIRE_CURRENT_FLAW(f) fire_current_flaw(f)
#define FIRE_CURRENT_RESOLVER(r) fire_current_resolver(r)
#else
#define FIRE_CURRENT_FLAW(f)
#define FIRE_CURRENT_RESOLVER(r)
#endif

namespace ratio
{
  class solver;
  class flaw;
  class resolver;

  class graph
  {
    friend class solver;

  public:
    graph(solver &s);
    virtual ~graph() = default;

    solver &get_solver() const noexcept { return s; }
    const semitone::var &get_gamma() const noexcept { return gamma; }

  private:
    void reset_gamma();

    virtual void check();

    /**
     * @brief Enqueues the given flaw in the graph.
     *
     * @param f The flaw to enqueue.
     */
    virtual void enqueue(flaw &) {}

    /**
     * @brief Propagates the costs of the given flaw.
     *
     * @param f The flaw to propagate the costs of.
     */
    virtual void propagate_costs(flaw &) {}

    /**
     * @brief Builds the graph.
     */
    virtual void build() {}

    /**
     * @brief Adds a layer to the graph.
     */
    virtual void add_layer() {}

#ifdef GRAPH_PRUNING
    /**
     * @brief Prunes the causal graph.
     */
    virtual void prune() {}
#endif

#ifdef GRAPH_REFINING
    /**
     * @brief Refines the causal graph.
     */
    virtual void refine() {}
#endif

    virtual void push() {}
    virtual void pop() {}

  protected:
    virtual void activated_flaw(flaw &) {}
    virtual void negated_flaw(flaw &f) { propagate_costs(f); }
    virtual void activated_resolver(resolver &) {}
    virtual void negated_resolver(resolver &r);

    void new_flaw(flaw_ptr f, const bool &enqueue = true) const noexcept;
    const std::unordered_map<semitone::var, std::vector<flaw_ptr>> &get_flaws() const noexcept;
    const std::unordered_set<flaw *> &get_active_flaws() const noexcept;
    const std::unordered_map<semitone::var, std::vector<resolver_ptr>> &get_resolvers() const noexcept;
    void expand_flaw(flaw &f);

    void set_cost(flaw &f, const utils::rational &cost) const noexcept;

    std::vector<std::vector<std::pair<semitone::lit, double>>> get_incs() const noexcept;

  protected:
    solver &s; // The solver this graph belongs to.
  private:
    semitone::var gamma; // The variable representing the validity of this graph..
  };

  using graph_ptr = utils::u_ptr<graph>;
} // namespace ratio
