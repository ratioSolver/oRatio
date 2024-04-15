#include <algorithm>
#include <cassert>
#include "flaw.hpp"
#include "resolver.hpp"
#include "solver.hpp"

namespace ratio
{
    flaw::flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, bool exclusive) noexcept : s(s), causes(causes), exclusive(exclusive), position(s.get_idl_theory().new_var()) {}

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
    }

#ifdef ENABLE_VISUALIZATION
    json::json to_json(const flaw &f) noexcept
    {
        json::json j_flaw{{"id", get_id(f)}, {"phi", to_string(f.get_phi())}, {"cost", to_json(f.get_estimated_cost())}, {"pos", std::to_string(f.get_solver().get_idl_theory().bounds(f.get_position()).first)}, {"data", f.get_data()}};
        switch (f.get_solver().get_sat().value(f.get_phi()))
        {
        case utils::True:
            j_flaw["status"] = "active";
            break;
        case utils::False:
            j_flaw["status"] = "forbidden";
            break;
        case utils::Undefined:
            j_flaw["status"] = "inactive";
            break;
        }
        json::json causes(json::json_type::array);
        for (const auto &c : f.get_causes())
            causes.push_back(get_id(c.get()));
        j_flaw["causes"] = std::move(causes);
        return j_flaw;
    }
#endif
} // namespace ratio
