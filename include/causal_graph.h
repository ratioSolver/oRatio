#pragma once

#include "oratio_export.h"
#include "rational.h"
#include <unordered_map>
#include <vector>

#ifdef BUILD_LISTENERS
#define FIRE_CURRENT_FLAW(f) fire_current_flaw(f)
#define FIRE_CURRENT_RESOLVER(r) fire_current_resolver(r)
#else
#define FIRE_CURRENT_FLAW(f)
#define FIRE_CURRENT_RESOLVER(r)
#endif

namespace ratio::solver
{
  class solver;
  class flaw;
  class resolver;

  class causal_graph
  {
    friend class solver;

  public:
    ORATIO_EXPORT causal_graph();
    causal_graph(const causal_graph &orig) = delete;
    virtual ~causal_graph() = default;

    inline solver &get_solver() const noexcept { return *slv; }
    inline const semitone::var &get_gamma() const noexcept { return gamma; }

    /**
     * @brief Get the estimated cost of the given resolver.
     *
     * @param r The resolver whose cost is going to be estimated.
     * @return semitone::rational The estimated cost of the given resolver.
     */
    virtual semitone::rational get_estimated_cost(const resolver &) const noexcept { return semitone::rational::ZERO; }

  private:
    ORATIO_EXPORT virtual void init(solver &slv);
    ORATIO_EXPORT void check();

    virtual void enqueue(flaw &) {}
    virtual void propagate_costs(flaw &) {}
    virtual void build() {} // builds the graph..
    virtual void prune() {} // prunes the current graph..

    virtual void add_layer() {} // adds a layer to the graph..

    virtual void push() {}
    virtual void pop() {}
    ORATIO_EXPORT virtual void activated_flaw(flaw &f);
    ORATIO_EXPORT virtual void negated_flaw(flaw &f);
    ORATIO_EXPORT virtual void activated_resolver(resolver &r);
    ORATIO_EXPORT virtual void negated_resolver(resolver &r);

  protected:
    void expand_flaw(flaw &f);

  private:
    solver *slv;         // the solver this causal-graph belongs to..
    semitone::var gamma; // this variable represents the validity of the current graph..
  };
} // namespace ratio::solver
