#pragma once

#include "flaw.h"
#include "resolver.h"

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

    /**
     * @brief Get the estimated cost of the given resolver.
     *
     * @param r The resolver whose cost is going to be estimated.
     * @return utils::rational The estimated cost of the given resolver.
     */
    virtual utils::rational get_estimated_cost(const resolver &) const noexcept { return utils::rational::ZERO; }

  private:
    void check();

    virtual void enqueue(flaw &) {}
    void expand_flaw(flaw &f);
    virtual void propagate_costs(flaw &) {}
    virtual void build() {} // builds the graph..
    virtual void prune() {} // prunes the current graph..

    virtual void add_layer() {} // adds a layer to the graph..

    virtual void push() {}
    virtual void pop() {}

  private:
    solver &s;           // The solver this graph belongs to.
    semitone::var gamma; // The variable representing the validity of this graph..
  };

  using graph_ptr = utils::u_ptr<graph>;
} // namespace ratio
