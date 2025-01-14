#include "solver_server.hpp"

namespace ratio::server
{
#if defined(SEMITONE)
    server::server() : network::server(), ratio::semitonesolver()
#elif defined(Z3)
    server::server() : network::server(), ratio::z3solver()
#endif
    {
    }

    void server::state_changed() {}
    void server::flaw_created(const ratio::flaw &f) {}
    void server::flaw_state_changed(const ratio::flaw &f) {}
    void server::flaw_cost_changed(const ratio::flaw &f) {}
    void server::flaw_position_changed(const ratio::flaw &f) {}
    void server::resolver_created(const ratio::resolver &r) {}
    void server::resolver_state_changed(const ratio::resolver &r) {}
    void server::causal_link_added(const ratio::flaw &f, const ratio::resolver &r) {}
} // namespace ratio::server
