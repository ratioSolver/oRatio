#include "solver_server.hpp"
#include "solver_api.hpp"
#include "logging.hpp"

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

        add_ws_route("/ratio").on_open(std::bind(&server::on_ws_open, this, network::placeholders::request)).on_close(std::bind(&server::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&server::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
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
        LOG_TRACE("New connection from " << ws.remote_endpoint());
        clients.insert(&ws);
        ws.send(make_solver_message(*this).dump());
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void server::on_ws_close(network::ws_session &ws)
    {
        LOG_TRACE("Connection closed with " << ws.remote_endpoint());
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void server::on_ws_error(network::ws_session &ws, const std::error_code &ec)
    {
        LOG_ERR("Error with " << ws.remote_endpoint() << ": " << ec.message());
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }

    void server::state_changed()
    {
        auto j_msg = riddle::core::to_json();
        j_msg["type"] = "state_changed";
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::flaw_created(const ratio::flaw &f)
    {
        auto j_msg = f.to_json();
        j_msg["type"] = "flaw_created";
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::flaw_state_changed(const ratio::flaw &f)
    {
        auto j_msg = json::json{{"type", "flaw_state_changed"}, {"id", f.get_id()}, {"state", to_string(f.get_state())}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::flaw_cost_changed(const ratio::flaw &f)
    {
        auto cost = f.get_estimated_cost();
        auto j_msg = json::json{{"type", "flaw_cost_changed"}, {"id", f.get_id()}, {"cost", {{"num", cost.numerator()}, {"den", cost.denominator()}}}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::flaw_position_changed(const ratio::flaw &f)
    {
        auto j_msg = json::json{{"type", "flaw_position_changed"}, {"id", f.get_id()}, {"position", f.get_position()}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::resolver_created(const ratio::resolver &r)
    {
        auto j_msg = r.to_json();
        j_msg["type"] = "resolver_created";
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::resolver_state_changed(const ratio::resolver &r)
    {
        auto j_msg = json::json{{"type", "resolver_state_changed"}, {"id", r.get_id()}, {"state", to_string(r.get_state())}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void server::causal_link_added(const ratio::flaw &f, const ratio::resolver &r)
    {
        auto j_msg = json::json{{"type", "causal_link_added"}, {"flaw", f.get_id()}, {"resolver", r.get_id()}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
} // namespace ratio::server
