#include "consumable_resource.h"
#include "solver.h"
#include "atom_flaw.h"
#include "combinations.h"
#include <cassert>

namespace ratio
{
    consumable_resource::consumable_resource(riddle::scope &scp) : smart_type(scp, CONSUMABLE_RESOURCE_NAME) {}

    void consumable_resource::new_atom(atom &atm)
    {
        if (atm.is_fact())
        {
            set_ni(atm.get_sigma());
            riddle::expr atm_expr(&atm);
            // we apply interval-predicate whenever the fact becomes active..
            get_solver().get_interval().call(atm_expr);
            restore_ni();
        }

        // we avoid unification..
        if (!get_solver().get_sat_core().new_clause({!atm.get_reason().get_phi(), atm.get_sigma()}))
            throw riddle::unsolvable_exception();

        // we store the variables for on-line flaw resolution..
        for (const auto &c_atm : atoms)
            store_variables(atm, *c_atm);

        atoms.emplace_back(&atm);
        // we store, for the atom, its atom listener..
        listeners.emplace_back(new cr_atom_listener(*this, atm));

        // we filter out those atoms which are not strictly active..
        if (get_solver().get_sat_core().value(atm.get_sigma()) == utils::True)
        {
            const auto c_scope = atm.get(TAU_KW);
            if (const auto enum_scope = dynamic_cast<enum_item *>(c_scope.operator->()))          // the `tau` parameter is a variable..
                for (const auto &val : get_solver().get_ov_theory().value(enum_scope->get_var())) // we check for all its allowed values..
                    to_check.insert(dynamic_cast<const riddle::item *>(val));
            else // the `tau` parameter is a constant..
                to_check.insert(&*c_scope);
        }
    }

    void consumable_resource::store_variables(atom &atm0, atom &atm1)
    {
        const auto a0_start = atm0.get(RATIO_START);
        const auto a0_end = atm0.get(RATIO_END);
        const auto a0_tau = atm0.get(TAU_KW);

        const auto a1_start = atm1.get(RATIO_START);
        const auto a1_end = atm1.get(RATIO_END);
        const auto a1_tau = atm1.get(TAU_KW);

        const auto a0_tau_itm = dynamic_cast<enum_item *>(a0_tau.operator->());
        const auto a1_tau_itm = dynamic_cast<enum_item *>(a1_tau.operator->());

        if (a0_tau_itm && a1_tau_itm)
        { // we have two, perhaps singleton, enum variables..
            const auto a0_vals = get_solver().get_ov_theory().value(a0_tau_itm->get_var());
            const auto a1_vals = get_solver().get_ov_theory().value(a1_tau_itm->get_var());

            bool found = false;
            for (const auto &v0 : a0_vals)
                if (a1_vals.count(v0))
                { // the two atoms can affect the same state variable..
                    if (!found)
                    { // we store the ordering variables..
#ifdef DL_TN
                        leqs[&atm0][&atm1] = get_solver().get_rdl_theory().new_leq(static_cast<arith_item &>(*a0_end).get_lin(), static_cast<arith_item &>(*a1_start).get_lin());
                        leqs[&atm1][&atm0] = get_solver().get_rdl_theory().new_leq(static_cast<arith_item &>(*a1_end).get_lin(), static_cast<arith_item &>(*a0_start).get_lin());
#elif LA_TN
                        leqs[&atm0][&atm1] = get_solver().get_lra_theory().new_leq(static_cast<arith_item &>(*a0_end).get_lin(), static_cast<arith_item &>(*a1_start).get_lin());
                        leqs[&atm1][&atm0] = get_solver().get_lra_theory().new_leq(static_cast<arith_item &>(*a1_end).get_lin(), static_cast<arith_item &>(*a0_start).get_lin());
                        // we boost propagation..
                        [[maybe_unused]] bool nc = get_solver().get_sat_core().new_clause({!leqs[&atm0][&atm1], !leqs[&atm1][&atm0]});
                        assert(nc);
#endif
                        found = true;
                        if (a0_vals.size() > 1 && a1_vals.size() > 1) // we store a variable for forcing a0 in v0 and a1 not in v0..
                            plcs[{&atm0, &atm1}].emplace_back(get_solver().get_sat_core().new_conj({get_solver().get_ov_theory().allows(a0_tau_itm->get_var(), *v0), !get_solver().get_ov_theory().allows(a1_tau_itm->get_var(), *v0)}), dynamic_cast<const riddle::item *>(v0));
                    }
                    assert(!plcs.count({&atm0, &atm1}) || plcs.at({&atm0, &atm1}).size() > 1);
                }
        }
        else if (a0_tau_itm)
        { // we have a singleton enum variable and a constant..
            if (const auto a0_vals = get_solver().get_ov_theory().value(a0_tau_itm->get_var()); a0_vals.count(static_cast<riddle::complex_item *>(a1_tau.operator->())))
            { // we store the ordering variables..
#ifdef DL_TN
                leqs[&atm0][&atm1] = get_solver().get_rdl_theory().new_leq(static_cast<arith_item &>(*a0_end).get_lin(), static_cast<arith_item &>(*a1_start).get_lin());
                leqs[&atm1][&atm0] = get_solver().get_rdl_theory().new_leq(static_cast<arith_item &>(*a1_end).get_lin(), static_cast<arith_item &>(*a0_start).get_lin());
#elif LA_TN
                leqs[&atm0][&atm1] = get_solver().get_lra_theory().new_leq(static_cast<arith_item &>(*a0_end).get_lin(), static_cast<arith_item &>(*a1_start).get_lin());
                leqs[&atm1][&atm0] = get_solver().get_lra_theory().new_leq(static_cast<arith_item &>(*a1_end).get_lin(), static_cast<arith_item &>(*a0_start).get_lin());
                // we boost propagation..
                [[maybe_unused]] bool nc = get_solver().get_sat_core().new_clause({!leqs[&atm0][&atm1], !leqs[&atm1][&atm0]});
                assert(nc);
#endif
            }
        }
        else if (a1_tau_itm)
        { // we have a singleton enum variable and a constant..
            if (const auto a1_vals = get_solver().get_ov_theory().value(a1_tau_itm->get_var()); a1_vals.count(static_cast<riddle::complex_item *>(a0_tau.operator->())))
            { // we store the ordering variables..
#ifdef DL_TN
                leqs[&atm0][&atm1] = get_solver().get_rdl_theory().new_leq(static_cast<arith_item &>(*a0_end).get_lin(), static_cast<arith_item &>(*a1_start).get_lin());
                leqs[&atm1][&atm0] = get_solver().get_rdl_theory().new_leq(static_cast<arith_item &>(*a1_end).get_lin(), static_cast<arith_item &>(*a0_start).get_lin());
#elif LA_TN
                leqs[&atm0][&atm1] = get_solver().get_lra_theory().new_leq(static_cast<arith_item &>(*a0_end).get_lin(), static_cast<arith_item &>(*a1_start).get_lin());
                leqs[&atm1][&atm0] = get_solver().get_lra_theory().new_leq(static_cast<arith_item &>(*a1_end).get_lin(), static_cast<arith_item &>(*a0_start).get_lin());
                // we boost propagation..
                [[maybe_unused]] bool nc = get_solver().get_sat_core().new_clause({!leqs[&atm0][&atm1], !leqs[&atm1][&atm0]});
                assert(nc);
#endif
            }
        }
        else if (a0_tau == a1_tau)
        { // we have two constants..
#ifdef DL_TN
            leqs[&atm0][&atm1] = get_solver().get_rdl_theory().new_leq(static_cast<arith_item &>(*a0_end).get_lin(), static_cast<arith_item &>(*a1_start).get_lin());
            leqs[&atm1][&atm0] = get_solver().get_rdl_theory().new_leq(static_cast<arith_item &>(*a1_end).get_lin(), static_cast<arith_item &>(*a0_start).get_lin());
#elif LA_TN
            leqs[&atm0][&atm1] = get_solver().get_lra_theory().new_leq(static_cast<arith_item &>(*a0_end).get_lin(), static_cast<arith_item &>(*a1_start).get_lin());
            leqs[&atm1][&atm0] = get_solver().get_lra_theory().new_leq(static_cast<arith_item &>(*a1_end).get_lin(), static_cast<arith_item &>(*a0_start).get_lin());
            // we boost propagation..
            [[maybe_unused]] bool nc = get_solver().get_sat_core().new_clause({!leqs[&atm0][&atm1], !leqs[&atm1][&atm0]});
            assert(nc);
#endif
        }
    }

    void consumable_resource::cr_atom_listener::something_changed()
    {
        if (cr.get_solver().get_sat_core().value(atm.get_sigma()) == utils::True)
        { // the atom is active..
            const auto a0_tau = atm.get(TAU_KW);
            cr.to_check.insert(&atm);
            if (const auto a0_tau_itm = dynamic_cast<enum_item *>(a0_tau.operator->())) // the `tau` parameter is a variable..
                for (const auto &val : cr.get_solver().get_ov_theory().value(a0_tau_itm->get_var()))
                    cr.to_check.insert(dynamic_cast<const riddle::item *>(val));
            else // the `tau` parameter is a constant..
                cr.to_check.insert(a0_tau.operator->());
        }
    }

    consumable_resource::cr_flaw::cr_flaw(consumable_resource &cr, const std::set<atom *> &atms) : flaw(cr.get_solver(), smart_type::get_resolvers(atms), true), cr(cr), overlapping_atoms(atms) {}

    json::json consumable_resource::cr_flaw::get_data() const noexcept
    {
        json::json data;
        data["type"] = "cr-flaw";

        json::json atoms(json::json_type::array);
        for (const auto &atm : overlapping_atoms)
            atoms.push_back(get_id(*atm));
        data["atoms"] = atoms;

        return data;
    }

    void consumable_resource::cr_flaw::compute_resolvers()
    {
        const auto cs = utils::combinations(std::vector<atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2);
        for (const auto &as : cs)
        {
            if (const auto a0_it = cr.leqs.find(as[0]); a0_it != cr.leqs.cend())
                if (const auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                    if (get_solver().get_sat_core().value(a0_a1_it->second) != utils::False)
                        add_resolver(new order_resolver(*this, a0_a1_it->second, *as[0], *as[1]));

            if (const auto a1_it = cr.leqs.find(as[1]); a1_it != cr.leqs.cend())
                if (const auto a1_a0_it = a1_it->second.find(as[0]); a1_a0_it != a1_it->second.cend())
                    if (get_solver().get_sat_core().value(a1_a0_it->second) != utils::False)
                        add_resolver(new order_resolver(*this, a1_a0_it->second, *as[1], *as[0]));

            const auto a0_tau = as[0]->get(TAU_KW);
            const auto a1_tau = as[1]->get(TAU_KW);
            enum_item *a0_tau_itm = dynamic_cast<enum_item *>(&*a0_tau);
            enum_item *a1_tau_itm = dynamic_cast<enum_item *>(&*a1_tau);
            if (a0_tau_itm && a1_tau_itm)
            { // we have two, perhaps singleton, enum variables..
                const auto a0_vals = get_solver().get_ov_theory().value(a0_tau_itm->get_var());
                const auto a1_vals = get_solver().get_ov_theory().value(a1_tau_itm->get_var());
                if (a0_vals.size() > 1 && a1_vals.size() > 1)
                { // we have two non-singleton variables..
                    for (const auto &a0_a1_disp : cr.plcs.at({as[0], as[1]}))
                        if (get_solver().get_sat_core().value(a0_a1_disp.first) == utils::Undefined)
                            add_resolver(new place_resolver(*this, a0_a1_disp.first, *as[0], *a0_a1_disp.second, *as[1]));
                }
                else if (a0_vals.size() > 1) // only `a1_tau` is a singleton variable..
                    add_resolver(new forbid_resolver(*this, *as[0], *a1_tau));
                else if (a1_vals.size() > 1) // only `a0_tau` is a singleton variable..
                    add_resolver(new forbid_resolver(*this, *as[1], *a0_tau));
            }
            else if (a0_tau_itm && get_solver().get_ov_theory().value(a0_tau_itm->get_var()).size() > 1) // only `a1_tau` is a singleton variable..
                add_resolver(new forbid_resolver(*this, *as[0], *a1_tau));
            else if (a1_tau_itm && get_solver().get_ov_theory().value(a1_tau_itm->get_var()).size() > 1) // only `a0_tau` is a singleton variable..
                add_resolver(new forbid_resolver(*this, *as[1], *a0_tau));
        }
    }

    consumable_resource::cr_flaw::order_resolver::order_resolver(cr_flaw &flw, const semitone::lit &r, const atom &before, const atom &after) : resolver(flw, r, utils::rational::ZERO), before(before), after(after) {}

    json::json consumable_resource::cr_flaw::order_resolver::get_data() const noexcept
    {
        json::json data;
        data["type"] = "order";
        data["before"] = get_id(before);
        data["after"] = get_id(after);
        return data;
    }

    consumable_resource::cr_flaw::place_resolver::place_resolver(cr_flaw &flw, const semitone::lit &r, atom &plc_atm, const riddle::item &plc_itm, atom &frbd_atm) : resolver(flw, r, utils::rational::ZERO), plc_atm(plc_atm), plc_itm(plc_itm), frbd_atm(frbd_atm) {}

    json::json consumable_resource::cr_flaw::place_resolver::get_data() const noexcept
    {
        json::json data;
        data["type"] = "place";
        data["place_atom"] = get_id(plc_atm);
        data["place_item"] = get_id(plc_itm);
        data["forbid_atom"] = get_id(frbd_atm);
        return data;
    }

    consumable_resource::cr_flaw::forbid_resolver::forbid_resolver(cr_flaw &flw, atom &atm, riddle::item &itm) : resolver(flw, semitone::lit(), utils::rational::ZERO), atm(atm), itm(itm) {}

    json::json consumable_resource::cr_flaw::forbid_resolver::get_data() const noexcept
    {
        json::json data;
        data["type"] = "forbid";
        data["forbid_atom"] = get_id(atm);
        data["forbid_item"] = get_id(itm);
        return data;
    }
} // namespace ratio
