#include "basic_solver.hpp"
#include "types.hpp"
#include "basic_flaws.hpp"
#include <cassert>

#ifdef ORATIO_ENABLE_LISTENERS
#define STATE_CHANGED() state_changed()
#define NEW_NODE(n) slv.node_created(n)
#define FLAW_CREATED(n, f) flaw_created(n, f)
#define RESOLVER_APPLIED(n, r) slv.resolver_applied(n, r)
#define INCONSISTENT_NODE(n) slv.inconsistent_node(n)
#define CURRENT_NODE(n) slv.current_node(n)
#else
#define STATE_CHANGED()
#define NEW_NODE(n)
#define FLAW_CREATED(n, f)
#define RESOLVER_APPLIED(n, r)
#define INCONSISTENT_NODE(n)
#define CURRENT_NODE(n)
#endif

namespace ratio
{
    node::node(basic_solver &slv, std::weak_ptr<node> parent) noexcept : slv(slv), parent(parent) {}

    double node::cost(std::shared_ptr<utils::node<double>>) const noexcept { return open_flaws.size(); }
    std::unordered_map<std::shared_ptr<utils::node<double>>, double> node::get_successors()
    {
        while (true)
        { // Select the open flaw with the least estimated cost..
            auto flw_it = std::min_element(open_flaws.begin(), open_flaws.end(), [](const auto &a, const auto &b)
                                           { return a->get_estimated_cost() < b->get_estimated_cost(); });
            auto c_flw = std::static_pointer_cast<flaw>(*flw_it);
            open_flaws.erase(flw_it);
            closed_flaws.insert(c_flw);
            // Compute the resolvers for the selected flaw..
            slv.compute_resolvers(*c_flw);
            switch (c_flw->get_resolvers().size())
            {
            case 0: // No resolvers available, return empty list..
                consistent = false;
                INCONSISTENT_NODE(*this);
                return {};
            case 1: // Only one resolver, apply it directly..
            {
                auto res = std::dynamic_pointer_cast<resolver>(c_flw->get_resolvers().front());
                if (slv.apply_resolver(*res))
                {
                    resolvers.push_back(res);
                    RESOLVER_APPLIED(*this, *res);
                    continue; // Continue to the next flaw..
                }
                else
                {
                    consistent = false;
                    INCONSISTENT_NODE(*this);
                    return {};
                }
            }
            default: // Create a successor for each resolver..
            {
                std::unordered_map<std::shared_ptr<utils::node<double>>, double> successors;
                for (const auto &res_ptr : c_flw->get_resolvers())
                {
                    auto res = std::dynamic_pointer_cast<resolver>(res_ptr);
                    auto succ = std::make_shared<node>(slv, shared_from_this());
                    // Copy open and closed flaws to the successor..
                    succ->open_flaws = open_flaws;
                    succ->closed_flaws = closed_flaws;
                    // Apply the resolver on the successor..
                    if (slv.apply_resolver(*res))
                    {
                        NEW_NODE(*succ);
                        succ->resolvers = resolvers;
                        succ->resolvers.push_back(res);
                        RESOLVER_APPLIED(*succ, *res);
                        successors.emplace(succ, res->get_intrinsic_cost().numerator() / static_cast<double>(res->get_intrinsic_cost().denominator()));
                    }
                }
                return successors;
            }
            }
        }
    }

    bool node::is_goal() const noexcept { return open_flaws.empty() && consistent; }

    json::json node::to_json() const noexcept
    {
        json::json j{{"id", get_id()}};
        if (parent.lock())
            j["parent"] = parent.lock()->get_id();
        j["consistent"] = consistent;
        json::json j_ress;
        for (const auto &res : resolvers)
            j_ress[std::to_string(res->get_id())] = res->to_json();
        j["resolvers"] = std::move(j_ress);
        json::json j_flaws;
        for (const auto &flw : open_flaws)
            j_flaws[std::to_string(flw->get_id())] = flw->to_json();
        j["flaws"] = std::move(j_flaws);
        return j;
    }

    basic_solver::basic_solver() noexcept : solver("oRatio Basic Solver"), utils::a_star<double>(std::make_shared<node>(*this))
    {
        read(INIT_STRING);

        add_type(std::make_unique<riddle::state_variable>(*this));
        add_type(std::make_unique<riddle::reusable_resource>(*this));
        add_type(std::make_unique<riddle::consumable_resource>(*this));
    }

    riddle::expr basic_solver::new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values)
    {
        assert(!values.empty());
        if (values.size() == 1)
        { // Single-valued enum
            assert(&values.front()->get_type() == &tp);
            return values.front();
        }
        else
        {
            std::vector<std::reference_wrapper<const utils::enum_val>> ev_refs;
            for (auto &ev_ptr : values)
                ev_refs.emplace_back(*ev_ptr);
            auto ev = ac_slv.new_var(ev_refs);
            // .. and create a new enum flaw to manage the variable..
            std::vector<std::shared_ptr<riddle::resolver>> causes;
            if (!static_cast<node &>(get_current_node()).resolvers.empty())
                causes.push_back(static_cast<node &>(get_current_node()).resolvers.back());
            auto ef = std::make_shared<enum_flaw>(*this, std::move(causes), tp, std::move(values), ev);
            FLAW_CREATED(get_current_node(), *ef);
            static_cast<node &>(get_current_node()).open_flaws.insert(ef);
            return ef->get_var();
        }
    }

    void basic_solver::new_clause(std::vector<riddle::bool_expr> &&exprs)
    {
        assert(!exprs.empty());
        if (exprs.size() == 1)
        { // if there is only one expression, just execute it..
            if (!assert_expr(exprs[0]))
                throw std::runtime_error("Unsatisfiable constraints");
        }
        else
        { // otherwise, create a new clause flaw..
            std::vector<std::shared_ptr<riddle::resolver>> causes;
            if (!static_cast<node &>(get_current_node()).resolvers.empty())
                causes.push_back(static_cast<node &>(get_current_node()).resolvers.back());
            auto cf = std::make_shared<clause_flaw>(*this, std::move(causes), std::move(exprs));
            FLAW_CREATED(get_current_node(), *cf);
            static_cast<node &>(get_current_node()).open_flaws.insert(cf);
        }
    }
    void basic_solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<std::shared_ptr<riddle::resolver>> causes;
        if (!static_cast<node &>(get_current_node()).resolvers.empty())
            causes.push_back(static_cast<node &>(get_current_node()).resolvers.back());
        auto df = std::make_shared<disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
        FLAW_CREATED(get_current_node(), *df);
        static_cast<node &>(get_current_node()).open_flaws.insert(df);
    }

    void basic_solver::solve()
    {
        if (!ac_slv.propagate() || !lin_slv.check())
            throw std::runtime_error("Unsatisfiable constraints");
        STATE_CHANGED();
        auto result_node = search();
        if (!result_node)
            throw std::runtime_error("No solution found");
    }

    json::json basic_solver::to_json() const
    {
        json::json j = core::to_json();
        json::json j_nodes;
        for (const auto &n : get_all_nodes())
            j_nodes[std::to_string(static_cast<const node &>(*n).get_id())] = static_cast<const node &>(*n).to_json();
        j["nodes"] = std::move(j_nodes);
        j["current_node"] = static_cast<const node &>(get_current_node()).get_id();
        return j;
    }

    riddle::atom_expr basic_solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args)
    {
        std::vector<std::shared_ptr<riddle::resolver>> causes;
        if (!static_cast<node &>(get_current_node()).resolvers.empty())
            causes.push_back(static_cast<node &>(get_current_node()).resolvers.back());
        auto af = std::make_shared<atom_flaw>(*this, std::move(causes), is_fact, pred, std::move(args), new_bool());
        FLAW_CREATED(get_current_node(), *af);
        static_cast<node &>(get_current_node()).open_flaws.insert(af);
        return af->get_atom();
    }
} // namespace ratio
