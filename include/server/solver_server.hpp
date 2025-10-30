#pragma once

#include "server.hpp"
#include "solver.hpp"

namespace ratio
{
  class server : public network::server, public solver
  {
  public:
    server();

  private:
    std::unique_ptr<network::response> index(const network::request &req);
    std::unique_ptr<network::response> assets(const network::request &req);

    void on_ws_open(network::ws_server_session_base &ws);
    void on_ws_close(network::ws_server_session_base &ws);
    void on_ws_error(network::ws_server_session_base &ws, const std::error_code &);

  private:
    void state_changed() override;
    void flaw_created(const ratio::flaw &f) override;
    void flaw_state_changed(const ratio::flaw &f) override;
    void flaw_cost_changed(const ratio::flaw &f) override;
    void flaw_position_changed(const ratio::flaw &f) override;
    void current_flaw(std::optional<std::reference_wrapper<ratio::flaw>>) override;
    void resolver_created(const ratio::resolver &r) override;
    void resolver_state_changed(const ratio::resolver &r) override;
    void causal_link_added(const ratio::flaw &f, const ratio::resolver &r) override;
    void current_resolver(std::optional<std::reference_wrapper<ratio::resolver>>) override;

  private:
    std::unordered_set<network::ws_server_session_base *> clients;
  };
} // namespace ratio
