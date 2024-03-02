#include "graph.hpp"
#include "solver.hpp"

namespace ratio
{
    graph::graph(solver &slv) noexcept : slv(slv) {}
} // namespace ratio