#pragma once

#include "graph.h"
#include <deque>
#include <unordered_set>

namespace ratio
{
  class enum_flaw;
  class atom_flaw;

  class h_2 : public graph
  {
  public:
    h_2(solver &s);

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
    void prune_enums();

    void visit(flaw &f);

    void negated_resolver(resolver &r) override;

    class h_2_flaw : public flaw
    {
    public:
      h_2_flaw(flaw &sub_f, resolver &r, resolver &mtx_r);

    private:
      void compute_resolvers() override;

      json::json get_data() const noexcept override;

      class h_2_resolver : public resolver
      {
      public:
        h_2_resolver(h_2_flaw &f, resolver &sub_r);

        void apply() override;

        json::json get_data() const noexcept override;

      private:
        resolver &sub_r;
      };

    private:
      flaw &sub_f;
      resolver &mtx_r;
    };
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

    resolver *c_res = nullptr;     // the current resolver..
    std::vector<flaw *> h_2_flaws; // the h_2 flaws..
#endif
  };
} // namespace ratio