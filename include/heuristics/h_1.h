#pragma once

#include "graph.h"
#include <deque>
#include <unordered_set>

namespace ratio
{
  class enum_flaw;
  class atom_flaw;

  class h_1 : public graph
  {
  public:
    h_1(solver &s);

  private:
    void enqueue(flaw &f) override;

    void propagate_costs(flaw &f) override;

    void build() override;
    void add_layer() override;

#ifdef GRAPH_PRUNING
    void prune() override;
#endif

#ifdef GRAPH_REFINING
    void refine() override;
    void check_landmarks();
    void prune_enums();
#endif

    bool is_deferrable(flaw &f); // checks whether the given flaw is deferrable..

  private:
    std::deque<flaw *> flaw_q;          // the flaw queue (for the graph building procedure)..
    std::unordered_set<flaw *> visited; // the visited flaws, for graph cost propagation (and deferrable flaws check)..
#ifdef GRAPH_PRUNING
    std::unordered_set<flaw *> already_closed; // already closed flaws (for avoiding duplicating graph pruning constraints)..
#endif
#ifdef GRAPH_REFINING
    std::vector<enum_flaw *> enum_flaws;       // the enum flaws..
    std::unordered_set<atom_flaw *> landmarks; // the possible landmarks..
#endif
  };
} // namespace ratio
