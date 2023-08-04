#include "h_2.h"
#include "solver.h"

namespace ratio
{
    meta_flaw::meta_flaw(flaw &f, std::vector<std::reference_wrapper<resolver>> cs, std::unordered_set<resolver *> ms) : flaw(f.get_solver(), std::move(cs)), f(f), mutexes(ms) {}

    void meta_flaw::compute_resolvers()
    {
        for (auto &res : f.get_resolvers())
            if (mutexes.find(&res.get()) == mutexes.end())
            {
                auto m_res = new meta_resolver(*this, res.get());
                add_resolver(m_res);
                for (auto &pre : res.get().get_preconditions())
                    new_causal_link(pre.get(), *m_res);
            }
    }

    meta_resolver::meta_resolver(meta_flaw &mf, const resolver &res) : resolver(mf, res.get_rho(), res.get_estimated_cost()), mf(mf), res(res) {}

    h_2::h_2(solver &s) : h_1(s) {}
} // namespace ratio
