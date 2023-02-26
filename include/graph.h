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

    virtual void activated_flaw(flaw &) {}
    virtual void negated_flaw(flaw &f) { propagate_costs(f); }
    virtual void activated_resolver(resolver &) {}
    virtual void negated_resolver(resolver &r);

  private:
    solver &s;           // The solver this graph belongs to.
    semitone::var gamma; // The variable representing the validity of this graph..
  };

  using graph_ptr = utils::u_ptr<graph>;
} // namespace ratio
