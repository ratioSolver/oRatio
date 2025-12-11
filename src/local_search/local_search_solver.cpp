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
        {
            std::vector<std::reference_wrapper<const utils::enum_val>> ev_refs;
            for (auto &ev_ptr : values)
                ev_refs.emplace_back(*ev_ptr);
            auto ev = ac_slv.new_var(ev_refs);
            // .. and create a new enum flaw to manage the variable..
            std::vector<std::shared_ptr<riddle::resolver>> causes;
            auto res = get_current_resolver();
            if (res)
                causes.push_back(res);
            auto ef = new_flaw<enum_flaw>(*this, std::move(causes), res ? std::dynamic_pointer_cast<ls_resolver>(res)->get_rho() : utils::TRUE_lit, tp, std::move(values), ev);
            FLAW_CREATED(*ef);
            return ef->get_var();
        }
    }
} // namespace ratio
