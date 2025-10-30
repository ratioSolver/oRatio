#include "solver_server.hpp"
#include "logging.hpp"

namespace ratio
{
    server::server()
    {
        add_route(network::Get, "^/$", std::bind(&server::index, this, network::placeholders::request));
        add_route(network::Get, "^(/assets/.+)|/.+\\.ico|/.+\\.png", std::bind(&server::assets, this, network::placeholders::request));

        add_ws_route("/ratio").on_open(std::bind(&server::on_ws_open, this, network::placeholders::request)).on_close(std::bind(&server::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&server::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
    }

    std::unique_ptr<network::response> server::index(const network::request &) { return std::make_unique<network::file_response>(CLIENT_DIR "/index.html"); }
    std::unique_ptr<network::response> server::assets(const network::request &req)
    {
        std::string target = req.get_target();
        if (target.find('?') != std::string::npos)
            target = target.substr(0, target.find('?'));
        return std::make_unique<network::file_response>(CLIENT_DIR + target);
    }

    void server::on_ws_open(network::ws_server_session_base &ws)
    {
        clients.insert(&ws);
        auto j_slv = to_json();
        j_slv["msg_type"] = "solver";
        ws.send(j_slv.dump());
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void server::on_ws_close(network::ws_server_session_base &ws)
    {
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void server::on_ws_error(network::ws_server_session_base &ws, const std::error_code &ec)
    {
        LOG_ERR("WebSocket error: " + ec.message());
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }

    void server::state_changed()
    {
        auto j_msg = riddle::core::to_json();
        j_msg["msg_type"] = "state_changed";
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::flaw_created(const ratio::flaw &f)
    {
        auto j_msg = f.to_json();
        j_msg["id"] = f.get_id();
        j_msg["msg_type"] = "flaw_created";
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::flaw_state_changed(const ratio::flaw &f)
    {
        auto j_msg = json::json{{"msg_type", "flaw_state_changed"}, {"id", f.get_id()}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::flaw_cost_changed(const ratio::flaw &f)
    {
        auto j_msg = json::json{{"msg_type", "flaw_cost_changed"}, {"id", f.get_id()}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::flaw_position_changed(const ratio::flaw &f)
    {
        auto j_msg = json::json{{"msg_type", "flaw_position_changed"}, {"id", f.get_id()}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::current_flaw(std::optional<std::reference_wrapper<ratio::flaw>> f)
    {
        auto j_msg = json::json{{"msg_type", "current_flaw"}};
        if (f)
            j_msg["id"] = f.value().get().get_id();
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::resolver_created(const ratio::resolver &r)
    {
        auto j_msg = r.to_json();
        j_msg["id"] = r.get_id();
        j_msg["msg_type"] = "resolver_created";
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::resolver_state_changed(const ratio::resolver &r)
    {
        auto j_msg = json::json{{"msg_type", "resolver_state_changed"}, {"id", r.get_id()}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::current_resolver(std::optional<std::reference_wrapper<ratio::resolver>> r)
    {
        auto j_msg = json::json{{"msg_type", "current_resolver"}};
        if (r)
            j_msg["id"] = r.value().get().get_id();
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::causal_link_added(const ratio::flaw &f, const ratio::resolver &r)
    {
        auto j_msg = json::json{{"msg_type", "causal_link_added"}, {"flaw", f.get_id()}, {"resolver", r.get_id()}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
} // namespace ratio
