#include "solver_server.hpp"
#include "flaws.hpp"
#include "logging.hpp"

namespace ratio
{
    solver_server::solver_server()
    {
        add_route(network::Get, "^/$", std::bind(&solver_server::index, this, network::placeholders::request));
        add_route(network::Get, "^(/assets/.+)|/.+\\.ico|/.+\\.png", std::bind(&solver_server::assets, this, network::placeholders::request));

        add_ws_route("/ratio").on_open(std::bind(&solver_server::on_ws_open, this, network::placeholders::request)).on_close(std::bind(&solver_server::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&solver_server::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
    }

    std::unique_ptr<network::response> solver_server::index(const network::request &) { return std::make_unique<network::file_response>(CLIENT_DIR "/index.html"); }
    std::unique_ptr<network::response> solver_server::assets(const network::request &req)
    {
        std::string target = req.get_target();
        if (target.find('?') != std::string::npos)
            target = target.substr(0, target.find('?'));
        return std::make_unique<network::file_response>(CLIENT_DIR + target);
    }

    void solver_server::on_ws_open(network::ws_server_session_base &ws)
    {
        clients.insert(&ws);
        auto j_slv = to_json();
        j_slv["msg_type"] = "solver";
        ws.send(j_slv.dump());
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void solver_server::on_ws_close(network::ws_server_session_base &ws)
    {
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void solver_server::on_ws_error(network::ws_server_session_base &ws, const std::error_code &ec)
    {
        LOG_ERR("WebSocket error: " + ec.message());
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }

    void solver_server::state_changed()
    {
        auto j_msg = riddle::core::to_json();
        j_msg["msg_type"] = "state_changed";
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }

    void solver_server::flaw_created(const riddle::flaw &f)
    {
        auto j_msg = f.to_json();
        j_msg["id"] = static_cast<uint64_t>(f.get_id());
        j_msg["msg_type"] = "flaw_created";
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void solver_server::flaw_state_changed(const riddle::flaw &f)
    {
        auto j_msg = json::json{{"msg_type", "flaw_state_changed"}, {"id", f.get_id()}};
        switch (sat_val(static_cast<const flaw &>(f).get_phi()))
        {
        case utils::True:
            j_msg["state"] = "active";
            break;
        case utils::False:
            j_msg["state"] = "forbidden";
            break;
        default:
            j_msg["state"] = "inactive";
            break;
        }
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void solver_server::flaw_cost_changed(const riddle::flaw &f)
    {
        auto j_msg = json::json{{"msg_type", "flaw_cost_changed"}, {"id", f.get_id()}, {"cost", riddle::to_json(f.get_estimated_cost())}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void solver_server::current_flaw(std::shared_ptr<riddle::flaw> f)
    {
        auto j_msg = json::json{{"msg_type", "current_flaw"}};
        if (f)
            j_msg["id"] = f->get_id();
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }

    void solver_server::resolver_created(const riddle::resolver &r)
    {
        auto j_msg = r.to_json();
        j_msg["id"] = r.get_id();
        j_msg["msg_type"] = "resolver_created";
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void solver_server::resolver_state_changed(const riddle::resolver &r)
    {
        auto j_msg = json::json{{"msg_type", "resolver_state_changed"}, {"id", r.get_id()}};
        switch (sat_val(dynamic_cast<const resolver &>(r).get_rho()))
        {
        case utils::True:
            j_msg["state"] = "applied";
            break;
        case utils::False:
            j_msg["state"] = "forbidden";
            break;
        default:
            j_msg["state"] = "unapplied";
            break;
        }
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
    void solver_server::current_resolver(std::shared_ptr<riddle::resolver> r)
    {
        auto j_msg = json::json{{"msg_type", "current_resolver"}};
        if (r)
            j_msg["id"] = r->get_id();
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }

    void solver_server::causal_link_added(const riddle::flaw &f, const riddle::resolver &r)
    {
        auto j_msg = json::json{{"msg_type", "causal_link_added"}, {"flaw", f.get_id()}, {"resolver", r.get_id()}};
        auto msg = j_msg.dump();
        for (auto client : clients)
            client->send(msg);
    }
} // namespace ratio
