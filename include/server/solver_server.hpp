#pragma once

#include "server.hpp"
#if defined(SEMITONE)
#include "semitonesolver.hpp"
#elif defined(Z3)
#include "z3solver.hpp"
#endif

namespace ratio::server
{
#if defined(SEMITONE)
  class server : public network::server, public ratio::semitonesolver
#elif defined(Z3)
  class server : public network::server, public ratio::z3solver
#endif
  {
  public:
    server();

  private:
    void state_changed() override;
    void flaw_created(const ratio::flaw &f) override;
    void flaw_state_changed(const ratio::flaw &f) override;
    void flaw_cost_changed(const ratio::flaw &f) override;
    void flaw_position_changed(const ratio::flaw &f) override;
    void resolver_created(const ratio::resolver &r) override;
    void resolver_state_changed(const ratio::resolver &r) override;
    void causal_link_added(const ratio::flaw &f, const ratio::resolver &r) override;
  };
} // namespace ratio::server
