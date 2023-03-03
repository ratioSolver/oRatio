#pragma once

#include "flaw.h"
#include "resolver.h"
#include <unordered_set>

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

    void check();

    virtual void enqueue(flaw &) {}
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

  protected:
    void new_flaw(flaw_ptr f, const bool &enqueue = true) const noexcept;
    const std::unordered_set<flaw *> &get_flaws() const noexcept;
    void expand_flaw(flaw &f);

    void set_cost(flaw &f, const utils::rational &cost) const noexcept;

    std::vector<flaw_ptr> flush_pending_flaws() const noexcept;
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_incs() const noexcept;

  protected:
    solver &s; // The solver this graph belongs to.
  private:
    semitone::var gamma; // The variable representing the validity of this graph..
  };

  using graph_ptr = utils::u_ptr<graph>;
} // namespace ratio
