#include <algorithm>
#include <cassert>
#include "flaw.hpp"
#include "resolver.hpp"
#include "solver.hpp"

namespace ratio
{
    flaw::flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, bool exclusive) noexcept : s(s), causes(causes), exclusive(exclusive), position(s.get_idl_theory().new_var())
    {
#ifdef ENABLE_API
        s.get_sat().add_listener(*this);
        s.get_idl_theory().add_listener(*this);

        listen_idl(position);
#endif
    }

    resolver &flaw::get_cheapest_resolver() const noexcept
    {
        return *std::min_element(resolvers.begin(), resolvers.end(), [](const auto &lhs, const auto &rhs)
                                 { return lhs.get().get_estimated_cost() < rhs.get().get_estimated_cost(); });
    }

    void flaw::init() noexcept
    {
        assert(!expanded);
        assert(s.get_sat().root_level());

        // this flaw's position must be greater than 0..
        [[maybe_unused]] bool add_distance = s.get_sat().new_clause({s.get_idl_theory().new_distance(position, 0, 0)});
        assert(add_distance);

        // we consider the causes of this flaw..
        std::vector<utils::lit> cs;
        cs.reserve(causes.size());
        for (auto &c : causes)
        {
            c.get().preconditions.push_back(*this); // this flaw is a precondition of its `c` cause..
            supports.push_back(c);                  // .. and it also supports the `c` cause..
            cs.push_back(c.get().get_rho());
            // we force this flaw to stay before the effects of its causes..
            [[maybe_unused]] bool dist = s.get_sat().new_clause({s.get_idl_theory().new_distance(c.get().get_flaw().position, position, -1)});
            assert(dist);
        }

        // we initialize the phi variable of this flaw as the conjunction of the flaw's causes' rho variables..
        phi = s.get_sat().new_conj(std::move(cs));
#ifdef ENABLE_API
        listen_sat(variable(phi));
#endif
    }

#ifdef ENABLE_API
    void flaw::on_sat_value_changed(VARIABLE_TYPE v) { s.flaw_state_changed(*this); }

    void flaw::on_idl_value_changed(VARIABLE_TYPE v) { s.flaw_position_changed(*this); }
#endif
} // namespace ratio
