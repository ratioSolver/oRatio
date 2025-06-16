#include "solver_api.hpp"
#include "graph.hpp"

namespace ratio::server
{
    json::json make_solver_message(const graph &g) noexcept
    {
        json::json j = g.to_json();
        j["msg_type"] = "solver";
        return j;
    }

    json::json make_solvers_message(const std::vector<std::reference_wrapper<graph>> &gs) noexcept
    {
        json::json j{{"msg_type", "solvers"}};
        for (const auto &g : gs)
            j[std::to_string(g.get().get_id())] = g.get().to_json();
        return j;
    }
} // namespace ratio::server
