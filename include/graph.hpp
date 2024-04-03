#pragma once

#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>
#include "flaw.hpp"
#include "resolver.hpp"

namespace ratio
{
  class graph
  {
    friend class solver;

  public:
    graph(solver &slv) noexcept;

  protected:
    /**
     * @brief Adds the given flaw to the graph.
     *
     * @param f The flaw to add.
     * @param enqueue Whether to enqueue the flaw.
     */
    void add_flaw(std::unique_ptr<flaw> &&f, bool enqueue = true) noexcept;
    /**
     * @brief Enqueues the given flaw in the graph.
     *
     * @param f The flaw to enqueue.
     */
    void enqueue(flaw &f) noexcept { flaw_q.push_back(f); }

  private:
    solver &slv;                                                                    // the solver this graph belongs to..
    std::unordered_map<VARIABLE_TYPE, std::vector<std::unique_ptr<flaw>>> phis;     // the phi variables (propositional variable to flaws) of the flaws..
    std::unordered_map<VARIABLE_TYPE, std::vector<std::unique_ptr<resolver>>> rhos; // the rho variables (propositional variable to resolver) of the resolvers..
    std::deque<std::reference_wrapper<flaw>> flaw_q;                                // the flaw queue (for the graph building procedure)..
    VARIABLE_TYPE gamma;                                                            // the variable representing the validity of this graph..
  };
} // namespace ratio