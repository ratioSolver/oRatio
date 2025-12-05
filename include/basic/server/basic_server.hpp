#pragma once

#include "server.hpp"
#include "basic_solver.hpp"

namespace ratio
{
  class basic_server : public network::server, public basic_solver
  {
  public:
    basic_server();

  private:
    std::unique_ptr<network::response> index(const network::request &req);
    std::unique_ptr<network::response> assets(const network::request &req);

    void on_ws_open(network::ws_server_session_base &ws);
    void on_ws_close(network::ws_server_session_base &ws);
    void on_ws_error(network::ws_server_session_base &ws, const std::error_code &);

    void state_changed() noexcept override;
    void node_created(const utils::node<double> &n) noexcept override;

    void flaw_created(const utils::node<double> &n, const riddle::flaw &f) noexcept override;
    void resolver_applied(const utils::node<double> &n, const riddle::resolver &r) noexcept override;

    void current_node(const utils::node<double> &n) noexcept override;
    void inconsistent_node(const utils::node<double> &n) noexcept override;

  private:
    std::unordered_set<network::ws_server_session_base *> clients;
  };
} // namespace ratio
