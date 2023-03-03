#pragma once

#include "graph.h"
#include <deque>
#include <unordered_set>

namespace ratio
{
  class h_1 final : public graph
  {
  public:
    h_1(solver &s);

  private:
    void enqueue(flaw &f) override;

    void propagate_costs(flaw &f) override;

    void build() override;

    void prune() override;

    void add_layer() override;

    bool is_deferrable(flaw &f); // checks whether the given flaw is deferrable..

  private:
    std::deque<flaw *> flaw_q;          // the flaw queue (for the graph building procedure)..
    std::unordered_set<flaw *> visited; // the visited flaws, for graph cost propagation (and deferrable flaws check)..
#ifdef GRAPH_PRUNING
    std::unordered_set<flaw *> already_closed; // already closed flaws (for avoiding duplicating graph pruning constraints)..
#endif
  };
} // namespace ratio
