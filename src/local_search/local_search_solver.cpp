#include "local_search_solver.hpp"
#include "local_search_flaws.hpp"
#include <cassert>

#ifdef ORATIO_ENABLE_LISTENERS
#define STATE_CHANGED() state_changed()
#define FLAW_CREATED(f) flaw_created(f)
#define FLAW_STATE_CHANGED(f) flaw_state_changed(f)
#define FLAW_COST_CHANGED(f) flaw_cost_changed(f)
#define RESOLVER_CREATED(r) resolver_created(r)
#define RESOLVER_STATE_CHANGED(r) resolver_state_changed(r)
#define NEW_CAUSAL_LINK(f, r) causal_link_added(f, r)
#define CURRENT_FLAW(f) current_flaw(f)
#define CURRENT_RESOLVER(r) current_resolver(r)
#else
#define STATE_CHANGED()
#define FLAW_CREATED(f)
#define FLAW_STATE_CHANGED(f)
#define FLAW_COST_CHANGED(f)
#define RESOLVER_CREATED(r)
#define RESOLVER_STATE_CHANGED(r)
#define NEW_CAUSAL_LINK(f, r)
#define CURRENT_FLAW(f)
#define CURRENT_RESOLVER(r)
#endif

namespace ratio
{
    local_search_solver::local_search_solver() noexcept : solver("oRatio Local Search Solver")
    {
        read(INIT_STRING);

        add_type(std::make_unique<state_variable>(*this));
        add_type(std::make_unique<reusable_resource>(*this));
        add_type(std::make_unique<consumable_resource>(*this));
    }

    riddle::expr local_search_solver::new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values)
    {
        assert(!values.empty());
        if (values.size() == 1)
        { // Single-valued enum
            assert(&values.front()->get_type() == &tp);
            return values.front();
        }
        else
        { // Multi-valued enum
            std::vector<std::reference_wrapper<const utils::enum_val>> ev_refs;
            for (auto &ev_ptr : values)
                ev_refs.emplace_back(*ev_ptr);
            auto ev = ac_slv.new_var(ev_refs);
            // .. and create a new enum flaw to manage the variable..
            std::vector<std::shared_ptr<riddle::resolver>> causes;
            auto res = get_current_resolver();
            if (res)
                causes.push_back(res);
            auto ef = new_flaw<enum_flaw>(*this, std::move(causes), tp, std::move(values), ev);
            FLAW_CREATED(*ef);
            return ef->get_var();
        }
    }

    void local_search_solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        assert(disjuncts.size() > 1);
        std::vector<std::shared_ptr<riddle::resolver>> causes;
        auto res = get_current_resolver();
        if (res)
            causes.push_back(res);
        [[maybe_unused]] auto df = new_flaw<disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
        FLAW_CREATED(*df);
    }
    void local_search_solver::new_clause(std::vector<riddle::bool_expr> &&exprs)
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
            auto res = get_current_resolver();
            if (res)
                causes.push_back(res);
            [[maybe_unused]] auto cf = new_flaw<clause_flaw>(*this, std::move(causes), std::move(exprs));
            FLAW_CREATED(*cf);
        }
    }

    void local_search_solver::solve()
    {
    }

    json::json local_search_solver::to_json() const
    {
        json::json j_graph = core::to_json();
        return j_graph;
    }

    riddle::atom_expr local_search_solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::term>, std::less<>> &&args)
    {
        std::vector<std::shared_ptr<riddle::resolver>> causes;
        auto res = get_current_resolver();
        if (res)
            causes.push_back(res);
        auto af = new_flaw<atom_flaw>(*this, std::move(causes), is_fact, pred, std::move(args), new_bool());
        FLAW_CREATED(*af);
        return af->get_atom();
    }

    void local_search_solver::new_clause(std::vector<utils::lit> &&lits) { ac_slv.add_constraint(ac_slv.new_clause(std::move(lits))); }
    utils::lbool local_search_solver::sat_val(const utils::lit &l) const noexcept { return ac_slv.sat_val(l); }

    state_variable::state_variable(local_search_solver &slv) noexcept : riddle::state_variable(slv) {}
    std::shared_ptr<riddle::flaw> state_variable::new_peak(std::vector<riddle::atom_expr> &&atms) noexcept { return std::make_shared<sv_peak>(static_cast<local_search_solver &>(get_core()), std::move(atms)); }

    reusable_resource::reusable_resource(local_search_solver &slv) noexcept : riddle::reusable_resource(slv) {}
    std::shared_ptr<riddle::flaw> reusable_resource::new_peak(std::vector<riddle::atom_expr> &&atms) noexcept { return std::make_shared<rr_peak>(static_cast<local_search_solver &>(get_core()), std::move(atms)); }

    consumable_resource::consumable_resource(local_search_solver &slv) noexcept : riddle::consumable_resource(slv) {}

    std::shared_ptr<riddle::flaw> consumable_resource::new_overproduction(std::vector<riddle::atom_expr> &&prod_atms, std::vector<riddle::atom_expr> &&cons_atms) noexcept { return std::make_shared<cr_overproduction>(static_cast<local_search_solver &>(get_core()), std::move(prod_atms), std::move(cons_atms)); }
    std::shared_ptr<riddle::flaw> consumable_resource::new_overconsumption(std::vector<riddle::atom_expr> &&cons_atms, std::vector<riddle::atom_expr> &&prod_atms) noexcept { return std::make_shared<cr_overconsumption>(static_cast<local_search_solver &>(get_core()), std::move(cons_atms), std::move(prod_atms)); }
} // namespace ratio
