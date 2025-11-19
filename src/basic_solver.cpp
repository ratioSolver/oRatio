#include "basic_solver.hpp"
#include "items.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio
{
    flaw::flaw(basic_solver &slv) noexcept : slv(slv) {}

    resolver::resolver(flaw &flw, utils::rational &&intrinsic_cost) noexcept : flw(flw), intrinsic_cost(std::move(intrinsic_cost)) {}

    basic_solver::basic_solver() noexcept : solver_core("oRatio Basic Solver") {}

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
            auto &ef = new_flaw<enum_flaw>(*this, std::make_shared<riddle::enum_item>(tp, std::move(values), ev));
            return ef.get_var();
        }
    }

    void basic_solver::solve() {}

    enum_flaw::enum_flaw(basic_solver &slv, riddle::enum_expr var) noexcept : flaw(slv), var(std::move(var)) {}

    void enum_flaw::compute_resolvers() {}
} // namespace ratio
