#include "solver_api.hpp"
#include "graph.hpp"

namespace ratio::server
{
    json::json make_solver_message(const graph &g) noexcept
    {
        json::json j = g.to_json();
        j["type"] = "solver";
        return j;
    }

    json::json make_solvers_message(const std::vector<utils::ref_wrapper<graph>> &gs) noexcept
    {
        json::json j{{"type", "solvers"}};
        for (const auto &g : gs)
            j[std::to_string(g->get_id())] = g->to_json();
        return j;
    }
} // namespace ratio::server
