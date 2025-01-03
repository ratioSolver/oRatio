#include "graph.hpp"

namespace ratio
{
    flaw::flaw(graph &gr, std::vector<std::reference_wrapper<resolver>> &&causes) : gr(gr), causes(causes) {}

    resolver::resolver(flaw &f, utils::rational &&intrinsic_cost) : f(f), intrinsic_cost(intrinsic_cost) { f.resolvers.push_back(*this); }

    graph::graph() {}
} // namespace ratio