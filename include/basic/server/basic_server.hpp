#pragma once

#include "server.hpp"
#include "basic_solver.hpp"

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

    void state_changed() noexcept override;
    void node_created(const node &n) noexcept override;
    void flaw_created(const node &n, const flaw &f) noexcept override;
    void current_node(std::optional<std::reference_wrapper<node>> n) noexcept override;

  private:
    std::unordered_set<network::ws_server_session_base *> clients;
  };
} // namespace ratio