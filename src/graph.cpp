#include "graph.hpp"
#include "solver.hpp"

namespace ratio
{
    graph::graph(solver &slv) noexcept : slv(slv) {}

    void graph::add_flaw(std::unique_ptr<flaw> &&f, bool enqueue) noexcept
    {
        auto &phi = phis[variable(f->get_phi())];
        phi.push_back(std::move(f));

        if (enqueue)
            this->enqueue(*phi.back());
        else
            slv.expand_flaw(*phi.back());
    }
} // namespace ratio