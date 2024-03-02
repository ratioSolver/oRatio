#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include "flaw.hpp"
#include "resolver.hpp"

namespace ratio
{
  class graph
  {
  public:
    graph(solver &slv) noexcept;

  private:
    solver &slv;                                                                    // the solver this graph belongs to..
    std::unordered_map<VARIABLE_TYPE, std::vector<std::unique_ptr<flaw>>> phis;     // the phi variables (propositional variable to flaws) of the flaws..
    std::unordered_map<VARIABLE_TYPE, std::vector<std::unique_ptr<resolver>>> rhos; // the rho variables (propositional variable to resolver) of the resolvers..
  };
} // namespace ratio