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

            bool has_conflict = false;
            std::set<atom *> overlapping_atoms;
            for (const auto &p : pulses)
            {
                if (const auto at_start_p = starting_atoms.find(p); at_start_p != starting_atoms.cend())
                    overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
                if (const auto at_end_p = ending_atoms.find(p); at_end_p != ending_atoms.cend())
                    for (const auto &a : at_end_p->second)
                        overlapping_atoms.erase(a);

                if (overlapping_atoms.size() > 1) // we have a 'peak'..
                {
                    has_conflict = true;
                    std::vector<std::pair<semitone::lit, double>> choices;
                    std::unordered_set<semitone::var> vars;
                    for (const auto &as : utils::combinations(std::vector<atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2))
                    { // state-variable MCSs are made of two atoms..
                        std::set<atom *> mcs(as.cbegin(), as.cend());
                        if (!sv_flaws.count(mcs))
                        { // we create a new state-variable flaw..
                            auto flw = new sv_flaw(*this, mcs);
                            sv_flaws.insert({mcs, flw});
                            store_flaw(flw); // we store the flaw for retrieval when at root-level..
                        }

                        const auto a0_start = as[0]->get(RATIO_START);
                        const auto a0_end = as[0]->get(RATIO_END);
                        const auto a1_start = as[1]->get(RATIO_START);
                        const auto a1_end = as[1]->get(RATIO_END);

                        if (auto a0_it = leqs.find(as[0]); a0_it != leqs.cend())
                            if (auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                                if (get_solver().get_sat_core().value(a0_a1_it->second) == utils::Undefined && vars.insert(variable(a0_a1_it->second)).second)
                                {
#ifdef DL_TN
                                    const auto [min, max] = get_solver().get_rdl_theory().distance(static_cast<arith_item &>(*a0_end).get_lin(), static_cast<arith_item &>(*a1_start).get_lin());
                                    const auto commit = is_infinite(min) || is_infinite(max) ? 0.5 : to_double((std::min(max.get_rational(), utils::rational::ZERO) - std::min(min.get_rational(), utils::rational::ZERO)) / (max.get_rational() - min.get_rational()));
#elif LA_TN
                                    const auto work = (get_solver().arith_value(a1_end).get_rational() - get_solver().arith_value(a1_start).get_rational()) * (get_solver().arith_value(a0_end).get_rational() - get_solver().arith_value(a1_start).get_rational());
                                    const auto commit = work == utils::rational::ZERO ? -std::numeric_limits<double>::max() : 1l - 1l / (static_cast<double>(work.numerator()) / work.denominator());
#endif
                                    choices.emplace_back(a0_a1_it->second, commit);
                                }

                        if (auto a1_it = leqs.find(as[1]); a1_it != leqs.cend())
                            if (auto a1_a0_it = a1_it->second.find(as[0]); a1_a0_it != a1_it->second.cend())
                                if (get_solver().get_sat_core().value(a1_a0_it->second) == utils::Undefined && vars.insert(variable(a1_a0_it->second)).second)
                                {
#ifdef DL_TN
                                    const auto [min, max] = get_solver().get_rdl_theory().distance(static_cast<arith_item &>(*a1_end).get_lin(), static_cast<arith_item &>(*a0_start).get_lin());
                                    const auto commit = is_infinite(min) || is_infinite(max) ? 0.5 : to_double((std::min(max.get_rational(), utils::rational::ZERO) - std::min(min.get_rational(), utils::rational::ZERO)) / (max.get_rational() - min.get_rational()));
#elif LA_TN
                                    const auto work = (get_solver().arith_value(a0_end).get_rational() - get_solver().arith_value(a0_start).get_rational()) * (get_solver().arith_value(a1_end).get_rational() - get_solver().arith_value(a0_start).get_rational());
                                    const auto commit = work == utils::rational::ZERO ? -std::numeric_limits<double>::max() : 1l - 1l / (static_cast<double>(work.numerator()) / work.denominator());
#endif
                                    choices.emplace_back(a1_a0_it->second, commit);
                                }

                        for (const auto atm : as)
                            if (auto atm_frbs = frbs.find(atm); atm_frbs != frbs.cend())
                            {
                                auto nr_possible_frbs = std::count_if(atm_frbs->second.cbegin(), atm_frbs->second.cend(), [&](const auto &atm_sv)
                                                                      { return get_solver().get_sat_core().value(atm_sv.second) == utils::Undefined; });
                                for (const auto &atm_sv : atm_frbs->second)
                                    if (get_solver().get_sat_core().value(atm_sv.second) == utils::Undefined && vars.insert(variable(atm_sv.second)).second)
                                        choices.emplace_back(atm_sv.second, 1l - 1l / nr_possible_frbs);
                            }
                    }
                    incs.emplace_back(choices);
                }
            }
            if (!has_conflict)
                to_check.erase(sv);
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
        const auto start = atm.get(RATIO_START);
        const auto end = atm.get(RATIO_END);

        for (const auto &c_atm : atoms)
        {
            const auto c_start = c_atm->get(RATIO_START);
            const auto c_end = c_atm->get(RATIO_END);

#ifdef DL_TN
            auto atm_before = get_solver().get_rdl_theory().new_leq(static_cast<arith_item &>(*end).get_lin(), static_cast<arith_item &>(*c_start).get_lin());
            auto atm_after = get_solver().get_rdl_theory().new_leq(static_cast<arith_item &>(*c_end).get_lin(), static_cast<arith_item &>(*start).get_lin());
#elif LA_TN
            auto atm_before = get_solver().get_lra_theory().new_leq(static_cast<arith_item &>(*end).get_lin(), static_cast<arith_item &>(*c_start).get_lin());
            auto atm_after = get_solver().get_lra_theory().new_leq(static_cast<arith_item &>(*c_end).get_lin(), static_cast<arith_item &>(*start).get_lin());
            // we boost propagation..
            [[maybe_unused]] bool nc = get_solver().get_sat_core().new_clause({!atm_before, !atm_after});
            assert(nc);
#endif
            if (get_solver().get_sat_core().value(atm_before) == utils::Undefined)
                leqs[&atm][c_atm] = atm_before;
            if (get_solver().get_sat_core().value(atm_after) == utils::Undefined)
                leqs[c_atm][&atm] = atm_after;
        }

        const auto tau = atm.get(TAU_KW);
        if (const auto tau_enum = dynamic_cast<enum_item *>(tau.operator->())) // the `tau` parameter is a variable..
            for (const auto &sv : get_solver().get_ov_theory().value(tau_enum->get_var()))
            {
                auto var = get_solver().get_ov_theory().allows(tau_enum->get_var(), *sv);
                if (get_solver().get_sat_core().value(var) == utils::Undefined)
                    frbs[&atm][dynamic_cast<riddle::item *>(sv)] = !var;
            }

        atoms.emplace_back(&atm);
        // we store, for the atom, its atom listener..
        listeners.emplace_back(new sv_atom_listener(*this, atm));

        // we filter out those atoms which are not strictly active..
        if (get_solver().get_sat_core().value(atm.get_sigma()) == utils::True)
        {
            if (const auto tau_enum = dynamic_cast<enum_item *>(tau.operator->()))             // the `tau` parameter is a variable..
                for (const auto &sv : get_solver().get_ov_theory().value(tau_enum->get_var())) // we check for all its allowed values..
                    to_check.insert(dynamic_cast<const riddle::item *>(sv));
            else // the `tau` parameter is a constant..
                to_check.insert(&*tau);
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

    state_variable::sv_flaw::sv_flaw(state_variable &sv, const std::set<atom *> &atms) : flaw(sv.get_solver(), smart_type::get_resolvers(atms)), sv(sv), overlapping_atoms(atms) {}

    json::json state_variable::sv_flaw::get_data() const noexcept
    {
        json::json data{{"type", "sv_flaw"}};

        json::json atms(json::json_type::array);
        for (const auto &atm : overlapping_atoms)
            atms.push_back(get_id(*atm));
        data["atoms"] = atms;

        return data;
    }

    void state_variable::sv_flaw::compute_resolvers()
    {
        std::unordered_set<semitone::var> vars;
        for (const auto &as : utils::combinations(std::vector<atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2))
        {
            if (const auto a0_it = sv.leqs.find(as[0]); a0_it != sv.leqs.cend())
                if (const auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                    if (get_solver().get_sat_core().value(a0_a1_it->second) != utils::False && vars.insert(variable(a0_a1_it->second)).second)
                        add_resolver(new order_resolver(*this, a0_a1_it->second, *as[0], *as[1]));

            if (const auto a1_it = sv.leqs.find(as[1]); a1_it != sv.leqs.cend())
                if (const auto a1_a0_it = a1_it->second.find(as[0]); a1_a0_it != a1_it->second.cend())
                    if (get_solver().get_sat_core().value(a1_a0_it->second) != utils::False && vars.insert(variable(a1_a0_it->second)).second)
                        add_resolver(new order_resolver(*this, a1_a0_it->second, *as[1], *as[0]));
        }
        for (const auto atm : overlapping_atoms)
            if (const auto frbs = sv.frbs.find(atm); frbs != sv.frbs.cend())
                for (const auto &sv : frbs->second)
                    if (get_solver().get_sat_core().value(sv.second) != utils::False && vars.insert(variable(sv.second)).second)
                        add_resolver(new forbid_resolver(*this, sv.second, *atm, *sv.first));
    }

    state_variable::sv_flaw::order_resolver::order_resolver(sv_flaw &flw, const semitone::lit &r, const atom &before, const atom &after) : resolver(flw, r, utils::rational::ZERO), before(before), after(after) {}

    json::json state_variable::sv_flaw::order_resolver::get_data() const noexcept { return {{"type", "order"}, {"before", get_id(before)}, {"after", get_id(after)}}; }

    state_variable::sv_flaw::forbid_resolver::forbid_resolver(sv_flaw &flw, const semitone::lit &r, atom &atm, riddle::item &itm) : resolver(flw, r, utils::rational::ZERO), atm(atm), itm(itm) {}

    json::json state_variable::sv_flaw::forbid_resolver::get_data() const noexcept { return {{"type", "forbid"}, {"forbid_atom", get_id(atm)}, {"forbid_item", get_id(itm)}}; }

    json::json state_variable::extract() const noexcept
    {
        json::json tls(json::json_type::array);
        // we partition atoms for each state-variable they might insist on..
        std::unordered_map<const riddle::complex_item *, std::vector<atom *>> sv_instances;
        for (const auto &sv : get_instances())
            sv_instances.emplace(static_cast<riddle::complex_item *>(sv.operator->()), std::vector<atom *>());

        for (const auto &atm : atoms)
            if (get_solver().get_sat_core().value(atm->get_sigma()) == utils::True) // we filter out those atoms which are not strictly active..
            {
                const auto sv = atm->get(TAU_KW); // we get the state-variable..
                if (auto enum_scope = dynamic_cast<enum_item *>(sv.operator->()))
                { // the `tau` parameter is a variable..
                    for (const auto &sv_val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        sv_instances.at(static_cast<const riddle::complex_item *>(sv_val)).emplace_back(atm);
                }
                else // the `tau` parameter is a constant..
                    sv_instances.at(static_cast<riddle::complex_item *>(sv.operator->())).emplace_back(atm);
            }

        for (const auto &[sv, atms] : sv_instances)
        {
            json::json tl{{"id", get_id(*sv)}, {"type", STATE_VARIABLE_NAME}};
#ifdef COMPUTE_NAMES
            tl["name"] = get_solver().guess_name(*sv);
#endif

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
                json::json j_val{{"from", to_json(*std::prev(p))}, {"to", to_json(*p)}};

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