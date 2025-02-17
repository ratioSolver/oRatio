#pragma once

#include "server.hpp"
#if defined(SEMITONE)
#include "stsolver.hpp"
#elif defined(Z3)
#include "z3solver.hpp"
#endif

namespace ratio::server
{
#if defined(SEMITONE)
  class server : public network::server, public ratio::stsolver
#elif defined(Z3)
  class server : public network::server, public ratio::z3solver
#endif
  {
  public:
    server(std::string_view assets_dir = "./gui/app/dist");

  private:
    utils::u_ptr<network::response> index(const network::request &req);
    utils::u_ptr<network::response> assets(const network::request &req);

    void on_ws_open(network::ws_session &ws);
    void on_ws_close(network::ws_session &ws);
    void on_ws_error(network::ws_session &ws, const std::error_code &);

  private:
    void state_changed() override;
    void flaw_created(const ratio::flaw &f) override;
    void flaw_state_changed(const ratio::flaw &f) override;
    void flaw_cost_changed(const ratio::flaw &f) override;
    void flaw_position_changed(const ratio::flaw &f) override;
    void current_flaw(std::optional<utils::ref_wrapper<ratio::flaw>>) override;
    void resolver_created(const ratio::resolver &r) override;
    void resolver_state_changed(const ratio::resolver &r) override;
    void causal_link_added(const ratio::flaw &f, const ratio::resolver &r) override;
    void current_resolver(std::optional<utils::ref_wrapper<ratio::resolver>>) override;

  private:
    const std::string assets_dir;
    std::unordered_set<network::ws_session *> clients;
  };
} // namespace ratio::server
