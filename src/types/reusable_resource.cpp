#include "reusable_resource.h"
#include "solver.h"
#include "atom_flaw.h"
#include <cassert>

namespace ratio
{
    reusable_resource::reusable_resource(riddle::scope &scp) : smart_type(scp, REUSABLE_RESOURCE_NAME) {}

    void reusable_resource::new_atom(atom &atm)
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
        listeners.emplace_back(new rr_atom_listener(*this, atm));

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

    void reusable_resource::store_variables(atom &atm0, atom &atm1)
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
} // namespace ratio