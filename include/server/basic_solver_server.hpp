#pragma once

#include "server.hpp"
#include "basic_solver.hpp"

namespace ratio
{
  class server : public network::server, public basic_solver
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
    void flaw_created(const flaw &f) noexcept override;
    void resolver_created(const ratio::resolver &r) noexcept override;

    void current_flaw(std::optional<std::reference_wrapper<ratio::flaw>> f) noexcept override;
    void current_resolver(std::optional<std::reference_wrapper<ratio::resolver>> r) noexcept override;

  private:
    std::unordered_set<network::ws_server_session_base *> clients;
  };
} // namespace ratio