#include "sttypes.hpp"
#include "stsolver.hpp"
#include <sstream>
#include <cassert>

namespace ratio
{
    atom_listener::atom_listener(stcomponent_type &ct, atom &atm) noexcept : prop_listener(static_cast<solver &>(atm.get_core())), la_listener(static_cast<solver &>(atm.get_core()).get_linear_arithmetic_theory()), ct(ct), atm(atm)
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

        if (static_cast<solver &>(atm.get_core()).value(atm.get_sigma()) == utils::True)
        {
            auto tau = atm.get(riddle::tau_kw);
            if (auto *ov = dynamic_cast<const riddle::enum_item *>(&*tau))
                for (const auto &v : atm.get_core().enum_value(*ov))
                    ct.to_check.emplace(static_cast<riddle::component *>(&*v));
            else
                ct.to_check.emplace(static_cast<riddle::component *>(tau.get()));
        }
    }
    void atom_listener::on_change(const utils::var &v) noexcept
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
    void atom_listener::on_arith_change(const utils::var &) noexcept
    {
        auto tau = atm.get(riddle::tau_kw);
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(&*tau))
            for (const auto &v : atm.get_core().enum_value(*ov))
                ct.to_check.emplace(static_cast<riddle::component *>(&*v));
        else
            ct.to_check.emplace(static_cast<riddle::component *>(tau.get()));
    }

    stcomponent_type::stcomponent_type(solver &slv) noexcept : slv(slv) {}
    utils::lbool stcomponent_type::share_component(riddle::atom_expr lhs, riddle::atom_expr rhs)
    {
        auto l_tau = lhs->get(riddle::tau_kw);
        auto r_tau = rhs->get(riddle::tau_kw);
        if (l_tau == r_tau)
            return utils::True; // the atoms must be on the same component..

        std::set<const riddle::component *> l_tau_cmps;
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(&*l_tau))
            for (const auto &v : get_solver().enum_value(*ov))
                l_tau_cmps.emplace(static_cast<riddle::component *>(&*v));
        else
            l_tau_cmps.emplace(static_cast<riddle::component *>(&*l_tau));

        std::set<const riddle::component *> r_tau_cmps;
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(&*r_tau))
            for (const auto &v : get_solver().enum_value(*ov))
                r_tau_cmps.emplace(static_cast<riddle::component *>(&*v));
        else
            r_tau_cmps.emplace(static_cast<riddle::component *>(&*r_tau));

        if (l_tau_cmps.size() == 1 && r_tau_cmps.size() == 1 && *l_tau_cmps.begin() == *r_tau_cmps.begin())
            return utils::True; // the atoms must be on the same component..
        for (const auto &r : r_tau_cmps)
            if (l_tau_cmps.count(r))
                return utils::Undefined; // the atoms might be on the same component..

        return utils::False; // the atoms can't be on the same component..
    }

    ststate_variable::ststate_variable(solver &slv) noexcept : state_variable(slv), stcomponent_type(slv) {}

    bool ststate_variable::solve_inconsistencies() { return false; }

    void ststate_variable::created_atom(riddle::atom_expr atm) noexcept
    {
        if (atm->is_fact())
            get_core().get_predicate(interval_kw).call(atm);

        // we store the variables for on-line flaw resolution..
        auto tau = atm->get(riddle::tau_kw);
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(&*tau))
            for (const auto &v : get_solver().enum_value(*ov))
                frbs[&*atm][&*v] = ov->get_lit(*v);

        const auto start = utils::s_ptr_cast<riddle::arith_item>(atm->get(riddle::start_kw));
        const auto end = utils::s_ptr_cast<riddle::arith_item>(atm->get(riddle::end_kw));
        for (const auto &c_atm : get_atoms())
            if (atm != c_atm)
            {
                switch (share_component(atm, c_atm))
                {
                case utils::True:
                { // the atoms are on the same state-variable..
                    const auto c_start = utils::s_ptr_cast<riddle::arith_item>(c_atm->get(riddle::start_kw));
                    const auto c_end = utils::s_ptr_cast<riddle::arith_item>(c_atm->get(riddle::end_kw));

                    if (get_solver().arith_ub(end) > get_solver().arith_lb(c_start) && get_solver().arith_lb(start) < get_solver().arith_ub(c_end))
                    { // the atoms might temporally overlap..
                        if (get_solver().arith_ub(start) < get_solver().arith_lb(c_end))
                            get_solver().add_le(end->get_lin(), c_start->get_lin()); // `atm` must be before `c_atm`..
                        else if (get_solver().arith_lb(end) > get_solver().arith_ub(c_start))
                            get_solver().add_le(c_end->get_lin(), start->get_lin()); // `c_atm` must be before `atm`..
                        else
                        { // the ordering constraints between the atoms are stored in the leqs map..
                            auto before = utils::lit(get_solver().mk_var());
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after);  // `c_atm` before `atm`..
                            assert(get_solver().value(before) == utils::Undefined && get_solver().value(after) == utils::Undefined);
                            leqs[&*atm][&*c_atm] = before;
                            leqs[&*c_atm][&*atm] = after;
                        }
                    }
                }
                break;
                case utils::Undefined:
                { // the atoms might be on the same state-variable..
                    const auto c_start = utils::s_ptr_cast<riddle::arith_item>(c_atm->get(riddle::start_kw));
                    const auto c_end = utils::s_ptr_cast<riddle::arith_item>(c_atm->get(riddle::end_kw));

                    if (get_solver().arith_ub(end) > get_solver().arith_lb(c_start) && get_solver().arith_lb(start) < get_solver().arith_ub(c_end))
                    { // the atoms might temporally overlap..
                        if (get_solver().arith_ub(start) < get_solver().arith_lb(c_end))
                        {
                            auto before = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            assert(get_solver().value(before) == utils::Undefined);
                            leqs[&*atm][&*c_atm] = before;
                        }
                        else if (get_solver().arith_lb(end) > get_solver().arith_ub(c_start))
                        {
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after); // `c_atm` before `atm`..
                            assert(get_solver().value(after) == utils::Undefined);
                            leqs[&*c_atm][&*atm] = after;
                        }
                        else
                        { // the ordering constraints between the atoms are stored in the leqs map..
                            auto before = utils::lit(get_solver().mk_var());
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after);  // `c_atm` before `atm`..
                            assert(get_solver().value(before) == utils::Undefined && get_solver().value(after) == utils::Undefined);
                            leqs[&*atm][&*c_atm] = before;
                            leqs[&*c_atm][&*atm] = after;
                        }
                    }
                }
                break;
                }
            }
    }

    streusable_resource::streusable_resource(solver &slv) noexcept : reusable_resource(slv), stcomponent_type(slv) {}

    bool streusable_resource::solve_inconsistencies() { return false; }

    void streusable_resource::created_atom(riddle::atom_expr atm) noexcept
    {
        if (atm->is_fact())
            get_core().get_predicate(interval_kw).call(atm);

        // we store the variables for on-line flaw resolution..
        auto tau = atm->get(riddle::tau_kw);
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(&*tau))
            for (const auto &v : get_solver().enum_value(*ov))
                frbs[&*atm][&*v] = ov->get_lit(*v);

        const auto start = utils::s_ptr_cast<riddle::arith_item>(atm->get(riddle::start_kw));
        const auto end = utils::s_ptr_cast<riddle::arith_item>(atm->get(riddle::end_kw));
        for (const auto &c_atm : get_atoms())
            if (atm != c_atm && share_component(atm, c_atm) != utils::False)
            { // the atoms might be on the same reusable-resource..
                const auto c_start = utils::s_ptr_cast<riddle::arith_item>(c_atm->get(riddle::start_kw));
                const auto c_end = utils::s_ptr_cast<riddle::arith_item>(c_atm->get(riddle::end_kw));

                if (get_solver().arith_ub(end) > get_solver().arith_lb(c_start) && get_solver().arith_lb(start) < get_solver().arith_ub(c_end))
                { // the atoms might temporally overlap..
                    if (get_solver().arith_ub(start) < get_solver().arith_lb(c_end))
                    {
                        auto before = utils::lit(get_solver().mk_var());
                        get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                        assert(get_solver().value(before) == utils::Undefined);
                        leqs[&*atm][&*c_atm] = before;
                    }
                    else if (get_solver().arith_lb(end) > get_solver().arith_ub(c_start))
                    {
                        auto after = utils::lit(get_solver().mk_var());
                        get_solver().add_le(c_end->get_lin(), start->get_lin(), after); // `c_atm` before `atm`..
                        assert(get_solver().value(after) == utils::Undefined);
                        leqs[&*c_atm][&*atm] = after;
                    }
                    else
                    { // the ordering constraints between the atoms are stored in the leqs map..
                        auto before = utils::lit(get_solver().mk_var());
                        auto after = utils::lit(get_solver().mk_var());
                        get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                        get_solver().add_le(c_end->get_lin(), start->get_lin(), after);  // `c_atm` before `atm`..
                        assert(get_solver().value(before) == utils::Undefined && get_solver().value(after) == utils::Undefined);
                        leqs[&*atm][&*c_atm] = before;
                        leqs[&*c_atm][&*atm] = after;
                    }
                }
            }
    }

    stconsumable_resource::stconsumable_resource(solver &slv) noexcept : consumable_resource(slv), stcomponent_type(slv) {}

    bool stconsumable_resource::solve_inconsistencies() { return false; }

    void stconsumable_resource::created_atom(riddle::atom_expr atm) noexcept
    {
        if (atm->is_fact())
            get_core().get_predicate(interval_kw).call(atm);

        // we store the variables for on-line flaw resolution..
        auto tau = atm->get(riddle::tau_kw);
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(&*tau))
            for (const auto &v : get_solver().enum_value(*ov))
                frbs[&*atm][&*v] = ov->get_lit(*v);

        const auto start = utils::s_ptr_cast<riddle::arith_item>(atm->get(riddle::start_kw));
        const auto end = utils::s_ptr_cast<riddle::arith_item>(atm->get(riddle::end_kw));
        for (const auto &c_atm : get_atoms())
            if (atm != c_atm && share_component(atm, c_atm) != utils::False)
            { // the atoms might be on the same consumable-resource..
                const auto c_start = utils::s_ptr_cast<riddle::arith_item>(c_atm->get(riddle::start_kw));
                const auto c_end = utils::s_ptr_cast<riddle::arith_item>(c_atm->get(riddle::end_kw));

                if (get_solver().arith_ub(end) > get_solver().arith_lb(c_start) && get_solver().arith_lb(start) < get_solver().arith_ub(c_end))
                { // the atoms might temporally overlap..
                    if (get_solver().arith_ub(start) < get_solver().arith_lb(c_end))
                    {
                        auto before = utils::lit(get_solver().mk_var());
                        get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                        assert(get_solver().value(before) == utils::Undefined);
                        leqs[&*atm][&*c_atm] = before;
                    }
                    else if (get_solver().arith_lb(end) > get_solver().arith_ub(c_start))
                    {
                        auto after = utils::lit(get_solver().mk_var());
                        get_solver().add_le(c_end->get_lin(), start->get_lin(), after); // `c_atm` before `atm`..
                        assert(get_solver().value(after) == utils::Undefined);
                        leqs[&*c_atm][&*atm] = after;
                    }
                    else
                    { // the ordering constraints between the atoms are stored in the leqs map..
                        auto before = utils::lit(get_solver().mk_var());
                        auto after = utils::lit(get_solver().mk_var());
                        get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                        get_solver().add_le(c_end->get_lin(), start->get_lin(), after);  // `c_atm` before `atm`..
                        assert(get_solver().value(before) == utils::Undefined && get_solver().value(after) == utils::Undefined);
                        leqs[&*atm][&*c_atm] = before;
                        leqs[&*c_atm][&*atm] = after;
                    }
                }
            }
    }
} // namespace ratio
