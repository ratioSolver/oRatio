#include "sttypes.hpp"
#include "stsolver.hpp"
#include <sstream>

namespace ratio
{
    statom_listener::statom_listener(stcomponent_type &ct, atom &atm) noexcept : prop_listener(static_cast<solver &>(atm.get_core())), la_listener(static_cast<solver &>(atm.get_core()).get_linear_arithmetic_theory()), ct(ct), atm(atm)
    {
        listen(variable(atm.get_sigma()));
        for (const auto &[name, xpr] : atm.items)
            if (auto *ov = dynamic_cast<const riddle::enum_item *>(&*xpr))
            {
                for (const auto &v : atm.get_core().enum_value(*ov))
                    listen(variable(ov->get_lit(*v)));
            }
            else if (xpr->get_type().get_name() == riddle::bool_kw)
            {
                if (static_cast<solver &>(atm.get_core()).value(static_cast<riddle::bool_item &>(*xpr).get_lit()) == utils::Undefined)
                    listen(variable(static_cast<riddle::bool_item &>(*xpr).get_lit()));
            }
            else if (xpr->get_type().get_name() == riddle::int_kw || xpr->get_type().get_name() == riddle::real_kw)
            {
                const auto lb = static_cast<solver &>(atm.get_core()).arith_lb(static_cast<riddle::arith_item &>(*xpr).get_lin());
                const auto ub = static_cast<solver &>(atm.get_core()).arith_ub(static_cast<riddle::arith_item &>(*xpr).get_lin());
                if (lb < ub)
                    for (const auto &l : static_cast<const riddle::arith_item &>(*xpr).get_lin().vars)
                        listen_arith(l.first);
            }
    }
    void statom_listener::on_change(const utils::var &v) noexcept
    {
        if (static_cast<solver &>(atm.get_core()).value(v) == utils::True)
        {
            auto tau = atm.get(riddle::tau_kw);
            if (auto *ov = dynamic_cast<const riddle::enum_item *>(&*tau))
                for (const auto &v : atm.get_core().enum_value(*ov))
                    ct.to_check.emplace(static_cast<riddle::component *>(&*v));
            else
                ct.to_check.emplace(static_cast<riddle::component *>(tau.get()));
        }
    }
    void statom_listener::on_arith_change(const utils::var &) noexcept
    {
        auto tau = atm.get(riddle::tau_kw);
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(&*tau))
            for (const auto &v : atm.get_core().enum_value(*ov))
                ct.to_check.emplace(static_cast<riddle::component *>(&*v));
        else
            ct.to_check.emplace(static_cast<riddle::component *>(tau.get()));
    }

    ststate_variable::ststate_variable(solver &slv) noexcept : state_variable(slv) {}

    bool ststate_variable::solve_inconsistencies() { return false; }

    streusable_resource::streusable_resource(solver &slv) noexcept : reusable_resource(slv) {}

    bool streusable_resource::solve_inconsistencies() { return false; }

    stconsumable_resource::stconsumable_resource(solver &slv) noexcept : consumable_resource(slv) {}

    bool stconsumable_resource::solve_inconsistencies() { return false; }
} // namespace ratio
