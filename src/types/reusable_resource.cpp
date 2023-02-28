#include "reusable_resource.h"
#include "solver.h"
#include "atom_flaw.h"
#include "combinations.h"
#include <cassert>

namespace ratio
{
    reusable_resource::reusable_resource(riddle::scope &scp) : smart_type(scp, REUSABLE_RESOURCE_NAME) {}

    std::vector<std::vector<std::pair<semitone::lit, double>>> reusable_resource::get_current_incs()
    {
        std::vector<std::vector<std::pair<semitone::lit, double>>> incs;
        // we partition atoms for each reusable-resource they might insist on..
        std::unordered_map<riddle::complex_item *, std::vector<atom *>> rr_instances;
        for (const auto &atm : atoms)
            if (get_solver().get_sat_core().value(atm->get_sigma()) == utils::True) // we filter out those atoms which are not strictly active..
            {
                const auto rr = atm->get(TAU_KW); // we get the reusable-resource..
                if (auto enum_scope = dynamic_cast<enum_item *>(rr.operator->()))
                { // the `tau` parameter is a variable..
                    for (const auto &rr_val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        if (to_check.count(static_cast<riddle::complex_item *>(rr_val))) // we consider only those reusable-resources which are still to be checked..
                            rr_instances[static_cast<riddle::complex_item *>(rr_val)].emplace_back(atm);
                }
                else if (to_check.count(static_cast<riddle::complex_item *>(rr.operator->()))) // we consider only those reusable-resources which are still to be checked..
                    rr_instances[static_cast<riddle::complex_item *>(rr.operator->())].emplace_back(atm);
            }

        // we detect inconsistencies for each of the reusable-resource instances..
        for (const auto &[rr, atms] : rr_instances)
        {
            // for each pulse, the atoms starting at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> ending_atoms;
            // all the pulses of the timeline..
            std::set<utils::inf_rational> pulses;
            // the resource capacity..
            auto c_capacity = get_solver().arith_value(rr->get(REUSABLE_RESOURCE_CAPACITY));

            for (const auto &atm : atms)
            {
                const auto start = get_solver().arith_value(atm->get(RATIO_START));
                const auto end = get_solver().arith_value(atm->get(RATIO_END));
                starting_atoms[start].insert(atm);
                ending_atoms[end].insert(atm);
                pulses.insert(start);
                pulses.insert(end);
            }

            std::set<atom *> overlapping_atoms;
            for (const auto &p : pulses)
            {
                if (const auto at_start_p = starting_atoms.find(p); at_start_p != starting_atoms.cend())
                    overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
                if (const auto at_end_p = ending_atoms.find(p); at_end_p != ending_atoms.cend())
                    for (const auto &a : at_end_p->second)
                        overlapping_atoms.erase(a);

                utils::inf_rational c_usage; // the concurrent resource usage..
                for (const auto &a : overlapping_atoms)
                    c_usage += get_solver().arith_value(a->get(REUSABLE_RESOURCE_USE_AMOUNT_NAME));

                if (c_usage > c_capacity) // we have a 'peak'..
                {
                    // we extract minimal conflict sets (MCSs)..
                    // we sort the overlapping atoms, according to their resource usage, in descending order..
                    std::vector<atom *> inc_atoms(overlapping_atoms.cbegin(), overlapping_atoms.cend());
                    std::sort(inc_atoms.begin(), inc_atoms.end(), [this](const auto &atm0, const auto &atm1)
                              { return get_solver().arith_value(atm0->get(REUSABLE_RESOURCE_USE_AMOUNT_NAME)) > get_solver().arith_value(atm1->get(REUSABLE_RESOURCE_USE_AMOUNT_NAME)); });

                    utils::inf_rational mcs_usage;       // the concurrent mcs resource usage..
                    auto mcs_begin = inc_atoms.cbegin(); // the beginning of the current mcs..
                    auto mcs_end = inc_atoms.cbegin();   // the end of the current mcs..
                    while (mcs_end != inc_atoms.cend())
                    {
                        // we increase the size of the current mcs..
                        while (mcs_usage <= c_capacity && mcs_end != inc_atoms.cend())
                        {
                            mcs_usage += get_solver().arith_value((*mcs_end)->get(REUSABLE_RESOURCE_USE_AMOUNT_NAME));
                            ++mcs_end;
                        }

                        if (mcs_usage > c_capacity)
                        { // we have a new mcs..
                            std::set<atom *> mcs(mcs_begin, mcs_end);
                            if (!rr_flaws.count(mcs))
                            { // we create a new reusable-resource flaw..
                                auto flw = new rr_flaw(*this, mcs);
                                rr_flaws.insert({mcs, flw});
                                store_flaw(flw); // we store the flaw for retrieval when at root-level..
                            }

                            std::vector<std::pair<semitone::lit, double>> choices;
                            for (const auto &as : utils::combinations(std::vector<atom *>(mcs_begin, mcs_end), 2))
                            {
                                std::vector<std::pair<semitone::lit, double>> choices;
                                const auto a0_start = as[0]->get(RATIO_START);
                                const auto a0_end = as[0]->get(RATIO_END);
                                const auto a1_start = as[1]->get(RATIO_START);
                                const auto a1_end = as[1]->get(RATIO_END);

                                if (auto a0_it = leqs.find(as[0]); a0_it != leqs.cend())
                                    if (auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                                        if (get_solver().get_sat_core().value(a0_a1_it->second) != utils::False)
                                        {
#ifdef DL_TN
                                            const auto [min, max] = get_solver().get_rdl_theory().distance(static_cast<arith_item &>(*a0_end).get_value(), static_cast<arith_item &>(*a1_start).get_value());
                                            const auto commit = is_infinite(min) || is_infinite(max) ? 0.5 : (std::min(to_double(max.get_rational()), 0.0) - std::min(to_double(min.get_rational()), 0.0)) / (to_double(max.get_rational()) - to_double(min.get_rational()));
#elif LA_TN
                                            const auto work = (get_solver().arith_value(a1_end).get_rational() - get_solver().arith_value(a1_start).get_rational()) * (get_solver().arith_value(a0_end).get_rational() - get_solver().arith_value(a1_start).get_rational());
                                            const auto commit = work == utils::rational::ZERO ? -std::numeric_limits<double>::max() : 1l - 1l / (static_cast<double>(work.numerator()) / work.denominator());
#endif
                                            choices.emplace_back(a0_a1_it->second, commit);
                                        }

                                if (auto a1_it = leqs.find(as[1]); a1_it != leqs.cend())
                                    if (auto a1_a0_it = a1_it->second.find(as[0]); a1_a0_it != a1_it->second.cend())
                                        if (get_solver().get_sat_core().value(a1_a0_it->second) != utils::False)
                                        {
#ifdef DL_TN
                                            const auto [min, max] = get_solver().get_rdl_theory().distance(static_cast<arith_item &>(*a1_end).get_value(), static_cast<arith_item &>(*a0_start).get_value());
                                            const auto commit = is_infinite(min) || is_infinite(max) ? 0.5 : (std::min(to_double(max.get_rational()), 0.0) - std::min(to_double(min.get_rational()), 0.0)) / (to_double(max.get_rational()) - to_double(min.get_rational()));
#elif LA_TN
                                            const auto work = (get_solver().arith_value(a0_end).get_rational() - get_solver().arith_value(a0_start).get_rational()) * (get_solver().arith_value(a1_end).get_rational() - get_solver().arith_value(a0_start).get_rational());
                                            const auto commit = work == utils::rational::ZERO ? -std::numeric_limits<double>::max() : 1l - 1l / (static_cast<double>(work.numerator()) / work.denominator());
#endif
                                            choices.emplace_back(a1_a0_it->second, commit);
                                        }

                                const auto a0_tau = as[0]->get(TAU_KW);
                                const auto a1_tau = as[1]->get(TAU_KW);

                                const auto a0_tau_itm = dynamic_cast<enum_item *>(a0_tau.operator->());
                                const auto a1_tau_itm = dynamic_cast<enum_item *>(a1_tau.operator->());
                                if (a0_tau_itm && a1_tau_itm)
                                { // we have two, perhaps singleton, enum variables..
                                    const auto a0_vals = get_solver().get_ov_theory().value(a0_tau_itm->get_var());
                                    const auto a1_vals = get_solver().get_ov_theory().value(a1_tau_itm->get_var());
                                    if (a0_vals.size() > 1 && a1_vals.size() > 1)
                                    { // we have two non-singleton variables..
                                        for (const auto &plc : plcs.at({as[0], as[1]}))
                                            if (get_solver().get_sat_core().value(plc.first) == utils::Undefined)
                                                choices.emplace_back(plc.first, 1l - 2l / static_cast<double>(a0_vals.size() + a1_vals.size()));
                                    }
                                    else if (a0_vals.size() > 1)
                                    { // only `a1_tau` is a singleton variable..
                                        if (get_solver().get_sat_core().value(get_solver().get_ov_theory().allows(static_cast<enum_item *>(a0_tau_itm)->get_var(), static_cast<riddle::complex_item &>(*a1_tau))) == utils::Undefined)
                                            choices.emplace_back(!get_solver().get_ov_theory().allows(static_cast<enum_item *>(a0_tau_itm)->get_var(), static_cast<riddle::complex_item &>(*a1_tau)), 1l - 1l / static_cast<double>(a0_vals.size()));
                                    }
                                    else if (a1_vals.size() > 1)
                                    { // only `a0_tau` is a singleton variable..
                                        if (get_solver().get_sat_core().value(get_solver().get_ov_theory().allows(static_cast<enum_item *>(a1_tau_itm)->get_var(), static_cast<riddle::complex_item &>(*a0_tau))) == utils::Undefined)
                                            choices.emplace_back(!get_solver().get_ov_theory().allows(static_cast<enum_item *>(a1_tau_itm)->get_var(), static_cast<riddle::complex_item &>(*a0_tau)), 1l - 1l / static_cast<double>(a1_vals.size()));
                                    }
                                }
                                else if (a0_tau_itm)
                                { // only `a1_tau` is a singleton variable..
                                    if (const auto a0_vals = get_solver().get_ov_theory().value(a0_tau_itm->get_var()); a0_vals.count(static_cast<riddle::complex_item *>(a1_tau.operator->())))
                                        if (get_solver().get_sat_core().value(get_solver().get_ov_theory().allows(static_cast<enum_item *>(a0_tau_itm)->get_var(), static_cast<riddle::complex_item &>(*a1_tau))) == utils::Undefined)
                                            choices.emplace_back(!get_solver().get_ov_theory().allows(static_cast<enum_item *>(a0_tau_itm)->get_var(), static_cast<riddle::complex_item &>(*a1_tau)), 1l - 1l / static_cast<double>(a0_vals.size()));
                                }
                                else if (a1_tau_itm)
                                { // only `a0_tau` is a singleton variable..
                                    if (const auto a1_vals = get_solver().get_ov_theory().value(a1_tau_itm->get_var()); a1_vals.count(static_cast<riddle::complex_item *>(a0_tau.operator->())))
                                        if (get_solver().get_sat_core().value(get_solver().get_ov_theory().allows(static_cast<enum_item *>(a1_tau_itm)->get_var(), static_cast<riddle::complex_item &>(*a0_tau))) == utils::Undefined)
                                            choices.emplace_back(!get_solver().get_ov_theory().allows(static_cast<enum_item *>(a1_tau_itm)->get_var(), static_cast<riddle::complex_item &>(*a0_tau)), 1l - 1l / static_cast<double>(a1_vals.size()));
                                }
                                incs.emplace_back(choices);

                                // we decrease the size of the mcs..
                                mcs_usage -= get_solver().arith_value((*mcs_begin)->get(REUSABLE_RESOURCE_USE_AMOUNT_NAME));
                                assert(mcs_usage <= c_capacity);
                                ++mcs_begin;
                            }
                        }
                    }
                }
            }
        }
        return incs;
    }

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

    void reusable_resource::rr_atom_listener::something_changed()
    {
        if (rr.get_solver().get_sat_core().value(atm.get_sigma()) == utils::True)
        { // the atom is active..
            const auto a0_tau = atm.get(TAU_KW);
            rr.to_check.insert(&atm);
            if (const auto a0_tau_itm = dynamic_cast<enum_item *>(a0_tau.operator->())) // the `tau` parameter is a variable..
                for (const auto &val : rr.get_solver().get_ov_theory().value(a0_tau_itm->get_var()))
                    rr.to_check.insert(dynamic_cast<const riddle::item *>(val));
            else // the `tau` parameter is a constant..
                rr.to_check.insert(a0_tau.operator->());
        }
    }
} // namespace ratio