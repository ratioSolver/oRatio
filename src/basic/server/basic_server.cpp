#include "basic_server.hpp"
#include "basic_flaws.hpp"
#include "logging.hpp"

namespace ratio
{
    basic_server::basic_server()
    {
        add_route(network::Get, "^/$", std::bind(&basic_server::index, this, network::placeholders::request));
        add_route(network::Get, "^(/assets/.+)|/.+\\.ico|/.+\\.png", std::bind(&basic_server::assets, this, network::placeholders::request));

        add_ws_route("/ratio").on_open(std::bind(&basic_server::on_ws_open, this, network::placeholders::request)).on_close(std::bind(&basic_server::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&basic_server::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
    }

    std::unique_ptr<network::response> basic_server::index(const network::request &) { return std::make_unique<network::file_response>(CLIENT_DIR "/index.html"); }
    std::unique_ptr<network::response> basic_server::assets(const network::request &req)
    {
        std::string target = req.get_target();
        if (target.find('?') != std::string::npos)
            target = target.substr(0, target.find('?'));
        return std::make_unique<network::file_response>(CLIENT_DIR + target);
    }

    void basic_server::on_ws_open(network::ws_server_session_base &ws)
    {
        clients.insert(&ws);
        auto j_slv = to_json();
        j_slv["msg_type"] = "solver";
        ws.send(j_slv.dump());
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void basic_server::on_ws_close(network::ws_server_session_base &ws)
    {
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void basic_server::on_ws_error(network::ws_server_session_base &ws, const std::error_code &ec)
    {
        LOG_ERR("WebSocket error: " + ec.message());
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }

    void basic_server::state_changed() noexcept
    {
        auto j_msg = to_json();
        j_msg["msg_type"] = "state_changed";
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }

    void basic_server::node_created(const utils::node<double> &n) noexcept
    {
        auto j_msg = static_cast<const node &>(n).to_json();
        j_msg["msg_type"] = "node_created";
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }

    void basic_server::flaw_created(const utils::node<double> &n, const riddle::flaw &f) noexcept
    {
        auto j_msg = f.to_json();
        j_msg["msg_type"] = "flaw_created";
        j_msg["node_id"] = static_cast<const node &>(n).get_id();
        j_msg["id"] = f.get_id();
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void basic_server::resolver_applied(const utils::node<double> &n, const riddle::resolver &r) noexcept
    {
        auto j_msg = r.to_json();
        j_msg["msg_type"] = "resolver_applied";
        j_msg["node_id"] = static_cast<const node &>(n).get_id();
        j_msg["id"] = r.get_id();
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }

    void basic_server::current_node(const utils::node<double> &n) noexcept
    {
        json::json j_msg;
        j_msg["msg_type"] = "current_node";
        j_msg["id"] = static_cast<const node &>(n).get_id();
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void basic_server::inconsistent_node(const utils::node<double> &n) noexcept
    {
        json::json j_msg;
        j_msg["msg_type"] = "inconsistent_node";
        j_msg["id"] = static_cast<const node &>(n).get_id();
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
} // namespace ratio
