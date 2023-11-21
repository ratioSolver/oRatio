#include "h_2.h"
#include "solver.h"
#include <cassert>

namespace ratio
{
    h_2::h_2(solver &s) : h_1(s) {}

#ifdef GRAPH_REFINING
    void h_2::refine()
    {
        // we refine the graph..
        h_1::refine();

        // we visit the flaws..
        for (auto &f : std::vector<flaw *>(get_active_flaws().begin(), get_active_flaws().end()))
            visit(*f);

        assert(s.get_sat_core().root_level());

        // we add the h_2 flaws..
        for (auto &f : h_2_flaws)
            new_flaw(f, false);
    }

    void h_2::visit(flaw &f)
    {
        // we visit the flaw..
        visited.insert(&f);

        // we check whether the best resolver is actually solvable..
        c_res = &f.get_best_resolver();
        s.take_decision(f.get_best_resolver().get_rho());

        // we visit the subflaws..
        for (auto &p : c_res->get_preconditions())
            if (!visited.count(&p.get()))
                visit(p.get());

        if (s.get_sat_core().value(c_res->get_rho()) != utils::True)
            s.get_sat_core().pop();

        // we unvisit the flaw..
        visited.erase(&f);
    }

    void h_2::negated_resolver(resolver &r)
    {
        // resolver c_res is mutex with r!

        // we refine the graph..
        h_1::negated_resolver(r);
    }

    h_2::h_2_flaw::h_2_flaw(flaw &sub_f, resolver &r, resolver &mtx_r) : flaw(sub_f.get_solver(), {r}), sub_f(sub_f), mtx_r(mtx_r) {}

    void h_2::h_2_flaw::compute_resolvers()
    {
        // we compute the resolvers..
        for (auto &r : sub_f.get_resolvers())
            if (&r.get() != &mtx_r)
                add_resolver(new h_2_resolver(*this, r.get()));
    }

    json::json h_2::h_2_flaw::get_data() const noexcept { return {{"type", "h_2"}, {"flaw", variable(sub_f.get_phi())}}; }

    h_2::h_2_flaw::h_2_resolver::h_2_resolver(h_2_flaw &f, resolver &sub_r) : resolver(f, sub_r.get_rho(), sub_r.get_intrinsic_cost()), sub_r(sub_r) {}

    void h_2::h_2_flaw::h_2_resolver::apply()
    {
        // we apply the resolver..
        for (auto &p : sub_r.get_preconditions())
            new_causal_link(p.get());
    }

    json::json h_2::h_2_flaw::h_2_resolver::get_data() const noexcept { return {{"type", "h_2"}, {"resolver", variable(sub_r.get_rho())}}; }
#endif
} // namespace ratio
