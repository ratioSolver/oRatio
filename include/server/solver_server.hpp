#pragma once

#include "server.hpp"
#include "solver.hpp"

namespace ratio
{
  class solver_server : public network::server, public solver
  {
  public:
    solver_server();

  private:
    std::unique_ptr<network::response> index(const network::request &req);
    std::unique_ptr<network::response> assets(const network::request &req);

    void on_ws_open(network::ws_server_session_base &ws);
    void on_ws_close(network::ws_server_session_base &ws);
    void on_ws_error(network::ws_server_session_base &ws, const std::error_code &);

    void state_changed() override;

    void flaw_created(const riddle::flaw &) override;
    void flaw_state_changed(const flaw &) override;
    void flaw_cost_changed(const riddle::flaw &) override;
    void current_flaw(std::shared_ptr<riddle::flaw>) override;

    void resolver_created(const riddle::resolver &) override;
    void resolver_state_changed(const resolver &) override;
    void current_resolver(std::shared_ptr<riddle::resolver>) override;

    void causal_link_added(const riddle::flaw &, const riddle::resolver &) override;

  private:
    std::unordered_set<network::ws_server_session_base *> clients;
  };
} // namespace ratio
