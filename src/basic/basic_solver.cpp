#include "basic_solver.hpp"
#include "types.hpp"
#include "basic_flaws.hpp"
#include <cassert>

namespace ratio
{
    basic_solver::basic_solver() noexcept : solver("oRatio Basic Solver"), utils::a_star<double>(std::make_shared<node>())
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
            static_cast<node &>(get_current_node()).open_flaws.insert(ef);
            return ef->get_var();
        }
    }
} // namespace ratio
