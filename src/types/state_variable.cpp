#include "state_variable.h"
#include "solver.h"
#include "combinations.h"
#include <cassert>

namespace ratio
{
    state_variable::state_variable(riddle::scope &scp) : smart_type(scp, STATE_VARIABLE_NAME) { add_constructor(new sv_constructor(*this)); }

    std::vector<std::vector<std::pair<semitone::lit, double>>> state_variable::get_current_incs()
    {
        std::vector<std::vector<std::pair<semitone::lit, double>>> incs;
        // we partition atoms for each state-variable they might insist on..
        std::unordered_map<const riddle::complex_item *, std::vector<atom *>> sv_instances;
        for (const auto &atm : atoms)
            if (get_solver().get_sat_core().value(atm->get_sigma()) == utils::True) // we filter out those atoms which are not strictly active..
            {
                const auto sv = atm->get(TAU_KW); // we get the state-variable..
                if (auto enum_scope = dynamic_cast<enum_item *>(sv.operator->()))
                { // the `tau` parameter is a variable..
                    for (const auto &sv_val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        if (to_check.count(static_cast<const riddle::complex_item *>(sv_val))) // we consider only those state-variables which are still to be checked..
                            sv_instances[static_cast<const riddle::complex_item *>(sv_val)].emplace_back(atm);
                }
                else if (to_check.count(static_cast<riddle::complex_item *>(sv.operator->()))) // we consider only those state-variables which are still to be checked..
                    sv_instances[static_cast<riddle::complex_item *>(sv.operator->())].emplace_back(atm);
            }

        // we detect inconsistencies for each of the state-variable instances..
        for ([[maybe_unused]] const auto &[sv, atms] : sv_instances)
        {
            // for each pulse, the atoms starting at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> ending_atoms;
            // all the pulses of the timeline..
            std::set<utils::inf_rational> pulses;

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

                if (overlapping_atoms.size() > 1) // we have a 'peak'..
                    for (const auto &as : utils::combinations(std::vector<atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2))
                    { // state-variable MCSs are made of two atoms..
                        std::set<atom *> mcs(as.cbegin(), as.cend());
                        if (!sv_flaws.count(mcs))
                        { // we create a new state-variable flaw..
                            auto flw = new sv_flaw(*this, mcs);
                            sv_flaws.insert({mcs, flw});
                            store_flaw(flw); // we store the flaw for retrieval when at root-level..
                        }

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
                    }
            }
        }
        return incs;
    }

    void state_variable::new_predicate(riddle::predicate &pred) noexcept { add_parent(pred, get_solver().get_interval()); }

    void state_variable::new_atom(atom &atm)
    {
        if (atm.is_fact())
        {
            set_ni(atm.get_sigma());
            riddle::expr atm_expr(&atm);
            // we apply interval-predicate whenever the fact becomes active..
            get_solver().get_interval().call(atm_expr);
            restore_ni();
        }

        // we store the variables for on-line flaw resolution..
        for (const auto &c_atm : atoms)
            store_variables(atm, *c_atm);

        atoms.emplace_back(&atm);
        // we store, for the atom, its atom listener..
        listeners.emplace_back(new sv_atom_listener(*this, atm));

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

    void state_variable::store_variables(atom &atm0, atom &atm1)
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

    void state_variable::sv_atom_listener::something_changed()
    {
        if (sv.get_solver().get_sat_core().value(atm.get_sigma()) == utils::True)
        { // the atom is active..
            const auto a0_tau = atm.get(TAU_KW);
            sv.to_check.insert(&atm);
            if (const auto a0_tau_itm = dynamic_cast<enum_item *>(a0_tau.operator->())) // the `tau` parameter is a variable..
                for (const auto &val : sv.get_solver().get_ov_theory().value(a0_tau_itm->get_var()))
                    sv.to_check.insert(dynamic_cast<const riddle::item *>(val));
            else // the `tau` parameter is a constant..
                sv.to_check.insert(a0_tau.operator->());
        }
    }

    state_variable::sv_flaw::sv_flaw(state_variable &sv, const std::set<atom *> &atms) : flaw(sv.get_solver(), smart_type::get_resolvers(atms), true), sv(sv), overlapping_atoms(atms) {}

    json::json state_variable::sv_flaw::get_data() const noexcept
    {
        json::json data;
        data["type"] = "sv_flaw";

        json::json atoms(json::json_type::array);
        for (const auto &atm : overlapping_atoms)
            atoms.push_back(get_id(*atm));
        data["atoms"] = atoms;

        return data;
    }

    void state_variable::sv_flaw::compute_resolvers()
    {
        const auto cs = utils::combinations(std::vector<atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2);
        for (const auto &as : cs)
        {
            if (const auto a0_it = sv.leqs.find(as[0]); a0_it != sv.leqs.cend())
                if (const auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                    if (get_solver().get_sat_core().value(a0_a1_it->second) != utils::False)
                        add_resolver(new order_resolver(*this, a0_a1_it->second, *as[0], *as[1]));

            if (const auto a1_it = sv.leqs.find(as[1]); a1_it != sv.leqs.cend())
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
                    for (const auto &a0_a1_disp : sv.plcs.at({as[0], as[1]}))
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

    state_variable::sv_flaw::order_resolver::order_resolver(sv_flaw &flw, const semitone::lit &r, const atom &before, const atom &after) : resolver(flw, r, utils::rational::ZERO), before(before), after(after) {}

    json::json state_variable::sv_flaw::order_resolver::get_data() const noexcept
    {
        json::json data;
        data["type"] = "order";
        data["before"] = get_id(before);
        data["after"] = get_id(after);
        return data;
    }

    state_variable::sv_flaw::place_resolver::place_resolver(sv_flaw &flw, const semitone::lit &r, atom &plc_atm, const riddle::item &plc_itm, atom &frbd_atm) : resolver(flw, r, utils::rational::ZERO), plc_atm(plc_atm), plc_itm(plc_itm), frbd_atm(frbd_atm) {}

    json::json state_variable::sv_flaw::place_resolver::get_data() const noexcept
    {
        json::json data;
        data["type"] = "place";
        data["place_atom"] = get_id(plc_atm);
        data["place_item"] = get_id(plc_itm);
        data["forbid_atom"] = get_id(frbd_atm);
        return data;
    }

    state_variable::sv_flaw::forbid_resolver::forbid_resolver(sv_flaw &flw, atom &atm, riddle::item &itm) : resolver(flw, semitone::lit(), utils::rational::ZERO), atm(atm), itm(itm) {}

    json::json state_variable::sv_flaw::forbid_resolver::get_data() const noexcept
    {
        json::json data;
        data["type"] = "forbid";
        data["forbid_atom"] = get_id(atm);
        data["forbid_item"] = get_id(itm);
        return data;
    }

    json::json state_variable::extract() const noexcept
    {
        json::json tls(json::json_type::array);
        // we partition atoms for each state-variable they might insist on..
        std::unordered_map<const riddle::complex_item *, std::vector<atom *>> sv_instances;
        for (const auto &atm : atoms)
            if (get_solver().get_sat_core().value(atm->get_sigma()) == utils::True) // we filter out those atoms which are not strictly active..
            {
                const auto sv = atm->get(TAU_KW); // we get the state-variable..
                if (auto enum_scope = dynamic_cast<enum_item *>(sv.operator->()))
                { // the `tau` parameter is a variable..
                    for (const auto &sv_val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        if (to_check.count(static_cast<const riddle::complex_item *>(sv_val))) // we consider only those state-variables which are still to be checked..
                            sv_instances[static_cast<const riddle::complex_item *>(sv_val)].emplace_back(atm);
                }
                else if (to_check.count(static_cast<riddle::complex_item *>(sv.operator->()))) // we consider only those state-variables which are still to be checked..
                    sv_instances[static_cast<riddle::complex_item *>(sv.operator->())].emplace_back(atm);
            }

        for (const auto &[sv, atms] : sv_instances)
        {
            json::json tl;
            tl["id"] = get_id(*sv);
#ifdef COMPUTE_NAMES
            tl["name"] = get_solver().guess_name(*sv);
#endif
            tl["type"] = STATE_VARIABLE_NAME;

            // for each pulse, the atoms starting at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> ending_atoms;
            // all the pulses of the timeline..
            std::set<utils::inf_rational> pulses;

            for (const auto &atm : atms)
            {
                const auto start = get_solver().arith_value(atm->get(RATIO_START));
                const auto end = get_solver().arith_value(atm->get(RATIO_END));
                starting_atoms[start].insert(atm);
                ending_atoms[end].insert(atm);
                pulses.insert(start);
                pulses.insert(end);
            }
            pulses.insert(get_solver().arith_value(get_solver().get("origin")));
            pulses.insert(get_solver().arith_value(get_solver().get("horizon")));

            std::set<atom *> overlapping_atoms;
            std::set<utils::inf_rational>::iterator p = pulses.begin();
            if (const auto at_start_p = starting_atoms.find(*p); at_start_p != starting_atoms.cend())
                overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
            if (const auto at_end_p = ending_atoms.find(*p); at_end_p != ending_atoms.cend())
                for (const auto &a : at_end_p->second)
                    overlapping_atoms.erase(a);

            json::json j_vals(json::json_type::array);
            for (p = std::next(p); p != pulses.end(); ++p)
            {
                json::json j_val;
                j_val["from"] = to_json(*std::prev(p));
                j_val["to"] = to_json(*p);

                json::json j_atms(json::json_type::array);
                for (const auto &atm : overlapping_atoms)
                    j_atms.push_back(get_id(*atm));
                j_val["atoms"] = std::move(j_atms);
                j_vals.push_back(std::move(j_val));

                if (const auto at_start_p = starting_atoms.find(*p); at_start_p != starting_atoms.cend())
                    overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
                if (const auto at_end_p = ending_atoms.find(*p); at_end_p != ending_atoms.cend())
                    for (const auto &a : at_end_p->second)
                        overlapping_atoms.erase(a);
            }
            tl["values"] = std::move(j_vals);

            tls.push_back(std::move(tl));
        }

        return tls;
    }
} // namespace ratio