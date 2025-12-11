#include "local_search_server.hpp"
#include "logging.hpp"

namespace ratio
{
    local_search_server::local_search_server()
    {
        add_route(network::Get, "^/$", std::bind(&local_search_server::index, this, network::placeholders::request));
        add_route(network::Get, "^(/assets/.+)|/.+\\.ico|/.+\\.png", std::bind(&local_search_server::assets, this, network::placeholders::request));

        add_ws_route("/ratio").on_open(std::bind(&local_search_server::on_ws_open, this, network::placeholders::request)).on_close(std::bind(&local_search_server::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&local_search_server::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
    }

    std::unique_ptr<network::response> local_search_server::index(const network::request &) { return std::make_unique<network::file_response>(CLIENT_DIR "/index.html"); }
    std::unique_ptr<network::response> local_search_server::assets(const network::request &req)
    {
        std::string target = req.get_target();
        if (target.find('?') != std::string::npos)
            target = target.substr(0, target.find('?'));
        return std::make_unique<network::file_response>(CLIENT_DIR + target);
    }

    void local_search_server::on_ws_open(network::ws_server_session_base &ws)
    {
        clients.insert(&ws);
        auto j_slv = to_json();
        j_slv["msg_type"] = "solver";
        ws.send(j_slv.dump());
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void local_search_server::on_ws_close(network::ws_server_session_base &ws)
    {
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void local_search_server::on_ws_error(network::ws_server_session_base &ws, const std::error_code &ec)
    {
        LOG_ERR("WebSocket error: " + ec.message());
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
} // namespace ratio
