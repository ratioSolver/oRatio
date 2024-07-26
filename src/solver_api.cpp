#include "solver_api.hpp"
#include "solver.hpp"
#include "graph.hpp"
#include "timeline.hpp"
#include <cassert>

namespace ratio
{
    [[nodiscard]] json::json to_json(const utils::rational &rat) noexcept
    {
        json::json j_rat;
        j_rat["num"] = rat.numerator();
        j_rat["den"] = rat.denominator();
        return j_rat;
    }

    [[nodiscard]] json::json to_json(const utils::inf_rational &rat) noexcept
    {
        json::json j_rat = to_json(rat.get_rational());
        if (rat.get_infinitesimal() != utils::rational::zero)
            j_rat["inf"] = to_json(rat.get_infinitesimal());
        return j_rat;
    }

    [[nodiscard]] json::json to_json(const solver &rhs) noexcept
    {
        std::set<riddle::item *> all_items; // we keep track of all the items..
        std::set<atom *> all_atoms;         // we keep track of all the atoms..
        std::queue<riddle::component_type *> q;
        for (const auto &tp : rhs.get_types())
            if (auto ct = dynamic_cast<riddle::component_type *>(&tp.get()))
                q.push(ct);
            else if (auto et = dynamic_cast<riddle::enum_type *>(&tp.get()))
                for (const auto &val : et->get_values())
                    all_items.insert(static_cast<riddle::item *>(val.get()));
        while (!q.empty())
        {
            for (const auto &i : q.front()->get_instances())
                all_items.insert(i.get());
            for (const auto &p : q.front()->get_predicates())
                for (const auto &atm : p.get().get_atoms())
                    all_atoms.insert(static_cast<atom *>(atm.get()));
            for (const auto &tp : q.front()->get_types())
                if (auto ct = dynamic_cast<riddle::component_type *>(&tp.get()))
                    q.push(ct);
                else if (auto et = dynamic_cast<riddle::enum_type *>(&tp.get()))
                    for (const auto &val : et->get_values())
                        all_items.insert(static_cast<riddle::item *>(val.get()));
            q.pop();
        }

        for (const auto &pred : rhs.get_predicates())
            for (const auto &atm : pred.get().get_atoms())
                all_atoms.insert(static_cast<atom *>(atm.get()));

        json::json j_solver{{"name", rhs.get_name()}};
        if (all_items.size())
        {
            json::json j_items(json::json_type::array);
            for (const auto &itm : all_items)
                j_items.push_back(to_json(*itm));
            j_solver["items"] = std::move(j_items);
        }
        if (all_atoms.size())
        {
            json::json j_atoms(json::json_type::array);
            for (const auto &atm : all_atoms)
                j_atoms.push_back(to_json(*atm));
            j_solver["atoms"] = std::move(j_atoms);
        }
        if (!rhs.get_items().empty())
        {
            json::json j_itms;
            for (const auto &[i_name, i] : rhs.get_items())
                j_itms[i_name] = value(*i);
            j_solver["exprs"] = std::move(j_itms);
        }
        return j_solver;
    }

    [[nodiscard]] json::json to_timelines(const solver &rhs) noexcept
    {
        json::json j_timelines(json::json_type::array);

        // for each pulse, the root atoms starting at that pulse..
        std::map<utils::inf_rational, std::set<atom *>> starting_atoms;
        // all the pulses of the solver timeline..
        std::set<utils::inf_rational> pulses;
        for (const auto &pred : rhs.get_predicates())
            for (const auto &atm : pred.get().get_atoms())
                if (is_impulse(static_cast<atom &>(*atm)))
                {
                    utils::inf_rational start = rhs.arithmetic_value(static_cast<riddle::arith_item &>(*atm->get("at")));
                    starting_atoms[start].insert(static_cast<atom *>(&*atm));
                    pulses.insert(start);
                }
                else if (is_interval(static_cast<atom &>(*atm)))
                {
                    utils::inf_rational start = rhs.arithmetic_value(static_cast<riddle::arith_item &>(*atm->get("start")));
                    starting_atoms[start].insert(static_cast<atom *>(&*atm));
                    pulses.insert(start);
                }
        if (!starting_atoms.empty())
        { // we have some root atoms in the solver timeline..
            json::json slv_tl{{"id", get_id(rhs)}, {"type", "Solver"}, {"name", rhs.get_name()}};
            json::json j_atms(json::json_type::array);
            for (const auto &p : pulses)
                for (const auto &atm : starting_atoms.at(p))
                    j_atms.push_back(get_id(*atm));
            slv_tl["values"] = std::move(j_atms);
            j_timelines.push_back(std::move(slv_tl));
        }

        std::queue<riddle::component_type *> q;
        for (const auto &tp : rhs.get_types())
            if (auto ct = dynamic_cast<riddle::component_type *>(&tp.get()))
                q.push(ct);
        while (!q.empty())
        {
            if (auto tl_tp = dynamic_cast<timeline *>(q.front())) // we have a timeline type..
            {                                                     // we extract the timeline..
                json::json j_tls = tl_tp->extract();
                for (size_t i = 0; i < j_tls.size(); ++i)
                    j_timelines.push_back(std::move(j_tls[i]));
            }
            for (const auto &tp : q.front()->get_types())
                if (auto ct = dynamic_cast<riddle::component_type *>(&tp.get()))
                    q.push(ct);
            q.pop();
        }

        return j_timelines;
    }

    [[nodiscard]] json::json to_json(const riddle::item &itm) noexcept
    {
        json::json j_itm{{"id", get_id(itm)}, {"name", itm.get_type().get_scope().get_core().guess_name(itm)}};
        // we add the full name of the type of the item..
        std::string tp_name = itm.get_type().get_name();
        const auto *t = &itm.get_type();
        while (const auto *et = dynamic_cast<const riddle::type *>(&t->get_scope()))
        {
            tp_name.insert(0, et->get_name() + ".");
            t = et;
        }
        j_itm["type"] = tp_name;

        if (auto ci = dynamic_cast<const riddle::component *>(&itm))
        {
            json::json j_itms;
            for (const auto &[i_name, i] : ci->get_items())
                j_itms[i_name] = value(*i);
            if (j_itms.size())
                j_itm["exprs"] = std::move(j_itms);
        }

        if (auto a = dynamic_cast<const riddle::atom *>(&itm))
        {
            j_itm["sigma"] = variable(a->get_sigma());
            switch (static_cast<solver &>(a->get_type().get_scope().get_core()).get_sat().value(a->get_sigma()))
            {
            case utils::True:
                j_itm["status"] = "Active";
                break;
            case utils::False:
                j_itm["status"] = "Unified";
                break;
            default:
                j_itm["status"] = "Inactive";
                break;
            }
            j_itm["is_fact"] = a->is_fact();
            json::json j_itms;
            for (const auto &[i_name, i] : a->get_items())
                j_itms[i_name] = value(*i);
            if (j_itms.size())
                j_itm["exprs"] = std::move(j_itms);
        }

        return j_itm;
    }

    [[nodiscard]] json::json value(const riddle::item &itm) noexcept
    {
        json::json j_val{{"type", itm.get_type().get_name()}}; // we add the type of the item..
        if (is_bool(itm))
        {
            j_val["type"] = itm.get_type().get_name();
            auto &b_itm = static_cast<const riddle::bool_item &>(itm);
            j_val["lit"] = (sign(b_itm.get_value()) ? "b" : "!b") + std::to_string(variable(b_itm.get_value()));
            switch (itm.get_type().get_scope().get_core().bool_value(b_itm))
            {
            case utils::True:
                j_val["val"] = "True";
                break;
            case utils::False:
                j_val["val"] = "False";
                break;
            default:
                j_val["val"] = "Undefined";
                break;
            }
            return j_val;
        }
        else if (is_arith(itm))
        {
            j_val["type"] = itm.get_type().get_name();
            auto &a_itm = static_cast<const riddle::arith_item &>(itm);
            j_val["lin"] = to_string(a_itm.get_value());
            j_val["val"] = to_json(itm.get_type().get_scope().get_core().arithmetic_value(a_itm));
            auto [lb, ub] = itm.get_type().get_scope().get_core().bounds(a_itm);
            if (!is_negative_infinite(lb))
                j_val["lb"] = to_json(lb);
            if (!is_positive_infinite(ub))
                j_val["ub"] = to_json(ub);
            return j_val;
        }
        else if (is_string(itm))
        {
            j_val["type"] = itm.get_type().get_name();
            j_val["val"] = static_cast<const riddle::string_item &>(itm).get_value();
            return j_val;
        }
        else if (is_enum(itm))
        {
            j_val["type"] = "enum";
            auto &e_itm = static_cast<const riddle::enum_item &>(itm);
            j_val["var"] = std::to_string(e_itm.get_value());
            json::json vals(json::json_type::array);
            for (auto &val : itm.get_type().get_scope().get_core().domain(e_itm))
                vals.push_back(get_id(dynamic_cast<riddle::item &>(val.get())));
            j_val["vals"] = std::move(vals);
            return j_val;
        }
        else
        {
            j_val["type"] = "item";
            j_val["val"] = get_id(itm);
            return j_val;
        }
    }

    [[nodiscard]] json::json to_json(const graph &rhs) noexcept
    {
        json::json j;
        json::json flaws(json::json_type::array);
        for (const auto &f : rhs.get_flaws())
            flaws.push_back(to_json(f.get()));
        j["flaws"] = std::move(flaws);
        if (rhs.get_current_flaw())
            j["current_flaw"] = to_json(rhs.get_current_flaw().value().get());
        json::json resolvers(json::json_type::array);
        for (const auto &r : rhs.get_resolvers())
            resolvers.push_back(to_json(r.get()));
        j["resolvers"] = std::move(resolvers);
        if (rhs.get_current_resolver())
            j["current_resolver"] = to_json(rhs.get_current_resolver().value().get());
        return j;
    }

    [[nodiscard]] json::json to_json(const flaw &f) noexcept
    {
        json::json j_flaw{{"id", get_id(f)}, {"state", to_state(f)}, {"phi", to_string(f.get_phi())}, {"cost", to_json(f.get_estimated_cost())}, {"pos", std::to_string(f.get_solver().get_idl_theory().bounds(f.get_position()).first)}, {"data", f.get_data()}};
        json::json causes(json::json_type::array);
        for (const auto &c : f.get_causes())
            causes.push_back(get_id(c.get()));
        j_flaw["causes"] = std::move(causes);
        return j_flaw;
    }

    [[nodiscard]] std::string to_state(const flaw &f) noexcept
    {
        switch (f.get_solver().get_sat().value(f.get_phi()))
        {
        case utils::True:
            return "active";
        case utils::False:
            return "forbidden";
        case utils::Undefined:
            return "inactive";
        }
        assert(false);
    }

    [[nodiscard]] json::json to_json(const resolver &r) noexcept
    {
        json::json j_r{{"id", get_id(r)}, {"state", to_state(r)}, {"flaw", get_id(r.get_flaw())}, {"rho", to_string(r.get_rho())}, {"intrinsic_cost", to_json(r.get_intrinsic_cost())}, {"data", r.get_data()}};
        json::json preconditions(json::json_type::array);
        for (const auto &p : r.get_preconditions())
            preconditions.push_back(get_id(p.get()));
        j_r["preconditions"] = std::move(preconditions);
        return j_r;
    }

    [[nodiscard]] std::string to_state(const resolver &r) noexcept
    {
        switch (r.get_flaw().get_solver().get_sat().value(r.get_rho()))
        {
        case utils::True:
            return "active";
        case utils::False:
            return "forbidden";
        default:
            return "inactive";
        }
    }

    [[nodiscard]] json::json make_solver_graph_message(const graph &g) noexcept
    {
        json::json j = to_json(g);
        j["type"] = "solver_graph";
        j["id"] = get_id(g.get_solver());
        return j;
    }

    [[nodiscard]] json::json make_flaw_created_message(const flaw &f) noexcept
    {
        json::json j = to_json(f);
        j["type"] = "flaw_created";
        j["solver_id"] = get_id(f.get_solver());
        return j;
    }

    [[nodiscard]] json::json make_flaw_state_changed_message(const flaw &f) noexcept
    {
        json::json j;
        j["type"] = "flaw_state_changed";
        j["solver_id"] = get_id(f.get_solver());
        j["id"] = get_id(f);
        j["state"] = to_state(f);
        return j;
    }

    [[nodiscard]] json::json make_flaw_cost_changed_message(const flaw &f) noexcept
    {
        json::json j;
        j["type"] = "flaw_cost_changed";
        j["solver_id"] = get_id(f.get_solver());
        j["id"] = get_id(f);
        j["cost"] = to_json(f.get_estimated_cost());
        return j;
    }

    [[nodiscard]] json::json make_flaw_position_changed_message(const flaw &f) noexcept
    {
        json::json j;
        j["type"] = "flaw_position_changed";
        j["solver_id"] = get_id(f.get_solver());
        j["id"] = get_id(f);
        j["position"] = f.get_solver().get_idl_theory().bounds(f.get_position()).first;
        return j;
    }

    [[nodiscard]] json::json make_current_flaw_message(const flaw &f) noexcept
    {
        json::json j;
        j["type"] = "current_flaw";
        j["solver_id"] = get_id(f.get_solver());
        j["id"] = get_id(f);
        return j;
    }

    [[nodiscard]] json::json make_resolver_created_message(const resolver &r) noexcept
    {
        json::json j = to_json(r);
        j["type"] = "resolver_created";
        j["solver_id"] = get_id(r.get_flaw().get_solver());
        return j;
    }

    [[nodiscard]] json::json make_resolver_state_changed_message(const resolver &r) noexcept
    {
        json::json j;
        j["type"] = "resolver_state_changed";
        j["solver_id"] = get_id(r.get_flaw().get_solver());
        j["id"] = get_id(r);
        j["state"] = to_state(r);
        return j;
    }

    [[nodiscard]] json::json make_current_resolver_message(const resolver &r) noexcept
    {
        json::json j;
        j["type"] = "current_resolver";
        j["solver_id"] = get_id(r.get_flaw().get_solver());
        j["id"] = get_id(r);
        return j;
    }

    [[nodiscard]] json::json make_causal_link_added_message(const flaw &f, const resolver &r) noexcept
    {
        json::json j;
        j["type"] = "causal_link_added";
        j["solver_id"] = get_id(f.get_solver());
        j["flaw_id"] = get_id(f);
        j["resolver_id"] = get_id(r);
        return j;
    }
} // namespace ratio
