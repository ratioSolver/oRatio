#include "flaw.h"
#include "solver.h"
#include "resolver.h"
#include "sat_core.h"
#include <cassert>

namespace ratio
{
    flaw::flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, const bool &exclusive) : s(s), causes(causes), exclusive(exclusive), position(s.idl_th.new_var()) {}

    void flaw::init() noexcept
    {
        assert(!expanded);
        assert(s.sat->root_level());

        // this flaw's position must be greater than 0..
        [[maybe_unused]] bool add_distance = s.get_sat_core().new_clause({s.idl_th.new_distance(position, 0, 0)});
        assert(add_distance);

        // we consider the causes of this flaw..
        std::vector<semitone::lit> cs;
        cs.reserve(causes.size());
        for (auto &c : causes)
        {
            c.get().preconditions.push_back(*this); // this flaw is a precondition of its `c` cause..
            supports.push_back(c);                  // .. and it also supports the `c` cause..
            cs.push_back(c.get().get_rho());
            // we force this flaw to stay before the effects of its causes..
            [[maybe_unused]] bool dist = s.get_sat_core().new_clause({s.idl_th.new_distance(c.get().f.position, position, -1)});
            assert(dist);
        }

        // we initialize the phi variable as the conjunction of the causes' rho variables..
        phi = s.get_sat_core().new_conj(std::move(cs));
    }

    void flaw::expand()
    {
        LOG("expanding flaw " << to_string(*this));
        assert(!expanded);
        assert(s.sat->root_level());

        expanded = true; // the flaw is now expanded..

        // we compute the resolvers..
        compute_resolvers();

        // we add causal relations between the flaw and its resolvers (i.e., if the flaw is phi exactly one of its resolvers should be in plan)..
        if (resolvers.empty())
        { // there is no way for solving this flaw: we force the phi variable at false..
            if (!s.get_sat_core().new_clause({!phi}))
                throw riddle::unsolvable_exception();
        }
        else
        { // we need to take a decision for solving this flaw..
            std::vector<semitone::lit> rs;
            rs.reserve(resolvers.size());
            rs.push_back(!phi);
            for (const auto &r : resolvers)
                rs.push_back(r.get().get_rho());

            // we link the phi variable to the resolvers' rho variables..
            if (!s.get_sat_core().new_clause(std::move(rs)))
                throw riddle::unsolvable_exception();

            if (exclusive) // we make the resolvers mutually exclusive..
                for (size_t i = 0; i < resolvers.size(); ++i)
                    for (size_t j = i + 1; j < resolvers.size(); ++j)
                        if (!s.get_sat_core().new_clause({!resolvers[i].get().rho, !resolvers[j].get().rho}))
                            throw riddle::unsolvable_exception();
        }
    }

    void flaw::add_resolver(resolver_ptr r)
    {
        if (!s.get_sat_core().new_clause({!r->get_rho(), phi}))
            throw riddle::unsolvable_exception();
        resolvers.push_back(*r);
        s.new_resolver(std::move(r)); // we notify the solver that a new resolver has been added..
    }

    resolver &flaw::get_cheapest_resolver() const noexcept
    {
        assert(!resolvers.empty());
        resolver *cheapest = &resolvers[0].get();
        utils::rational cheapest_cost = cheapest->get_estimated_cost();
        for (size_t i = 1; i < resolvers.size(); ++i)
        {
            resolver &r = resolvers[i].get();
            utils::rational cost = resolvers[i].get().get_estimated_cost();
            if (cost < cheapest_cost)
            {
                cheapest = &r;
                cheapest_cost = cost;
            }
        }
        return *cheapest;
    }

    std::string to_string(const flaw &f) noexcept
    {
        std::string state;
        switch (f.get_solver().get_sat_core().value(f.get_phi()))
        {
        case utils::True: // the flaw is active..
            state = "active";
            break;
        case utils::False: // the flaw is forbidden..
            state = "forbidden";
            break;
        default: // the flaw is inactive..
            state = "inactive";
            break;
        }
        return "ϕ" + std::to_string(variable(f.get_phi())) + " (" + state + ")";
    }

    json::json to_json(const flaw &f) noexcept
    {
        json::json j_f;
        j_f["id"] = get_id(f);
        json::json causes(json::json_type::array);
        for (const auto &c : f.get_causes())
            causes.push_back(get_id(c.get()));
        j_f["causes"] = std::move(causes);
        j_f["phi"] = to_string(f.get_phi());
        j_f["state"] = f.get_solver().get_sat_core().value(f.get_phi());
        j_f["cost"] = to_json(f.get_estimated_cost());
        j_f["pos"] = to_json(f.get_solver().get_idl_theory().bounds(f.get_position()));
        j_f["data"] = f.get_data();
        return j_f;
    }
} // namespace ratio
