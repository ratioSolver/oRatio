#include "solver_server.hpp"

namespace ratio::server
{
#if defined(SEMITONE)
    server::server() : network::server(SERVER_HOST, SERVER_PORT, 1), ratio::semitonesolver()
#elif defined(Z3)
    server::server(std::string_view assets_dir) : network::server(SERVER_HOST, SERVER_PORT, 1), ratio::z3solver(), assets_dir(assets_dir)
#endif
    {
        add_route(network::Get, "^/$", std::bind(&server::index, this, network::placeholders::request));
        add_route(network::Get, "^(/assets/.+)|/.+\\.ico|/.+\\.png", std::bind(&server::assets, this, network::placeholders::request));

        add_ws_route("/ratio").on_open(std::bind(&server::on_ws_open, this, network::placeholders::request)).on_message(std::bind(&server::on_ws_message, this, std::placeholders::_1, std::placeholders::_2)).on_close(std::bind(&server::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&server::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
    }

    std::unique_ptr<network::response> server::index(const network::request &)
    {
        return std::make_unique<network::file_response>(assets_dir + "/index.html");
    }
    std::unique_ptr<network::response> server::assets(const network::request &req)
    {
        std::string target = req.get_target();
        if (target.find('?') != std::string::npos)
            target = target.substr(0, target.find('?'));
        return std::make_unique<network::file_response>(assets_dir + target);
    }

    void server::on_ws_open(network::ws_session &ws)
    {
    }
    void server::on_ws_message(network::ws_session &ws, std::string_view msg)
    {
    }
    void server::on_ws_close(network::ws_session &ws)
    {
    }
    void server::on_ws_error(network::ws_session &ws, const std::error_code &)
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
