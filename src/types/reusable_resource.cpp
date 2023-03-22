#include "reusable_resource.h"
#include "solver.h"
#include "atom_flaw.h"
#include "combinations.h"
#include <cassert>

namespace ratio
{
    reusable_resource::reusable_resource(riddle::scope &scp) : smart_type(scp, REUSABLE_RESOURCE_NAME)
    {
        // we add the `capacity` parameter..
        add_field(new riddle::field(get_solver().get_real_type(), REUSABLE_RESOURCE_CAPACITY));

        // we create the constructor's arguments..
        std::vector<riddle::field_ptr> ctr_args;
        ctr_args.push_back(new riddle::field(get_solver().get_real_type(), REUSABLE_RESOURCE_CAPACITY));

        // we create the constructor's initialization list..
        ctr_ins.emplace_back(0, 0, 0, 0, REUSABLE_RESOURCE_CAPACITY);
        std::vector<riddle::ast::expression_ptr> ivs;
        ivs.push_back(new riddle::ast::id_expression({riddle::id_token(0, 0, 0, 0, REUSABLE_RESOURCE_CAPACITY)}));
        ctr_ivs.emplace_back(std::move(ivs));

        // we create the constructor's body..
        ctr_body.emplace_back(new riddle::ast::expression_statement(new riddle::ast::geq_expression(new riddle::ast::id_expression({riddle::id_token(0, 0, 0, 0, REUSABLE_RESOURCE_CAPACITY)}), new riddle::ast::real_literal_expression({riddle::real_token(0, 0, 0, 0, utils::rational::ZERO)}))));

        // we add the constructor..
        add_constructor(new riddle::constructor(*this, std::move(ctr_args), ctr_ins, ctr_ivs, ctr_body));

        // we create the `Use` predicate's arguments..
        std::vector<riddle::field_ptr> use_args;
        use_args.push_back(new riddle::field(get_solver().get_real_type(), REUSABLE_RESOURCE_AMOUNT_NAME));

        // we create the `Use` predicate's body..
        pred_body.emplace_back(new riddle::ast::expression_statement(new riddle::ast::geq_expression(new riddle::ast::id_expression({riddle::id_token(0, 0, 0, 0, REUSABLE_RESOURCE_AMOUNT_NAME)}), new riddle::ast::real_literal_expression({riddle::real_token(0, 0, 0, 0, utils::rational::ZERO)}))));

        // we add the `Use` predicate..
        u_pred = new riddle::predicate(*this, REUSABLE_RESOURCE_USE_PREDICATE_NAME, std::move(use_args), pred_body);
        add_parent(*u_pred, get_solver().get_interval());
        add_predicate(u_pred);
    }

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

            bool has_conflict = false;
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
                    c_usage += get_solver().arith_value(a->get(REUSABLE_RESOURCE_AMOUNT_NAME));

                if (c_usage > c_capacity) // we have a 'peak'..
                {
                    has_conflict = true;
                    // we extract minimal conflict sets (MCSs)..
                    // we sort the overlapping atoms, according to their resource usage, in descending order..
                    std::vector<atom *> inc_atoms(overlapping_atoms.cbegin(), overlapping_atoms.cend());
                    std::sort(inc_atoms.begin(), inc_atoms.end(), [this](const auto &atm0, const auto &atm1)
                              { return get_solver().arith_value(atm0->get(REUSABLE_RESOURCE_AMOUNT_NAME)) > get_solver().arith_value(atm1->get(REUSABLE_RESOURCE_AMOUNT_NAME)); });

                    utils::inf_rational mcs_usage;       // the concurrent mcs resource usage..
                    auto mcs_begin = inc_atoms.cbegin(); // the beginning of the current mcs..
                    auto mcs_end = inc_atoms.cbegin();   // the end of the current mcs..
                    while (mcs_end != inc_atoms.cend())
                    {
                        // we increase the size of the current mcs..
                        while (mcs_usage <= c_capacity && mcs_end != inc_atoms.cend())
                        {
                            mcs_usage += get_solver().arith_value((*mcs_end)->get(REUSABLE_RESOURCE_AMOUNT_NAME));
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
                            std::unordered_set<semitone::var> vars;
                            for (const auto &as : utils::combinations(std::vector<atom *>(mcs_begin, mcs_end), 2))
                            {
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
                            }
                            for (const auto &atm : mcs)
                                if (auto atm_frbs = frbs.find(atm); atm_frbs != frbs.cend())
                                {
                                    auto nr_possible_frbs = std::count_if(atm_frbs->second.cbegin(), atm_frbs->second.cend(), [&](const auto &atm_rr)
                                                                          { return get_solver().get_sat_core().value(atm_rr.second) == utils::Undefined; });
                                    for (const auto &atm_rr : atm_frbs->second)
                                        if (get_solver().get_sat_core().value(atm_rr.second) == utils::Undefined && vars.insert(variable(atm_rr.second)).second)
                                            choices.emplace_back(atm_rr.second, 1l - 1l / nr_possible_frbs);
                                }

                            // we decrease the size of the mcs..
                            mcs_usage -= get_solver().arith_value((*mcs_begin)->get(REUSABLE_RESOURCE_AMOUNT_NAME));
                            assert(mcs_usage <= c_capacity);
                            ++mcs_begin;

                            incs.emplace_back(choices);
                        }
                    }
                }
            }
            if (!has_conflict)
                to_check.erase(rr);
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
            for (const auto &rr : get_solver().get_ov_theory().value(tau_enum->get_var()))
            {
                auto var = get_solver().get_ov_theory().allows(tau_enum->get_var(), *rr);
                if (get_solver().get_sat_core().value(var) == utils::Undefined)
                    frbs[&atm][dynamic_cast<riddle::item *>(rr)] = !var;
            }

        atoms.emplace_back(&atm);
        // we store, for the atom, its atom listener..
        listeners.emplace_back(new rr_atom_listener(*this, atm));

        // we filter out those atoms which are not strictly active..
        if (get_solver().get_sat_core().value(atm.get_sigma()) == utils::True)
        {
            if (const auto tau_enum = dynamic_cast<enum_item *>(tau.operator->()))             // the `tau` parameter is a variable..
                for (const auto &rr : get_solver().get_ov_theory().value(tau_enum->get_var())) // we check for all its allowed values..
                    to_check.insert(dynamic_cast<const riddle::item *>(rr));
            else // the `tau` parameter is a constant..
                to_check.insert(&*tau);
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

    reusable_resource::rr_flaw::rr_flaw(reusable_resource &rr, const std::set<atom *> &atms) : flaw(rr.get_solver(), smart_type::get_resolvers(atms)), rr(rr), overlapping_atoms(atms) {}

    json::json reusable_resource::rr_flaw::get_data() const noexcept
    {
        json::json data{{"type", "rr-flaw"}};

        json::json atms(json::json_type::array);
        for (const auto &atm : overlapping_atoms)
            atms.push_back(get_id(*atm));
        data["atoms"] = atms;

        return data;
    }

    void reusable_resource::rr_flaw::compute_resolvers()
    {
        std::unordered_set<semitone::var> vars;
        for (const auto &as : utils::combinations(std::vector<atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2))
        {
            if (const auto a0_it = rr.leqs.find(as[0]); a0_it != rr.leqs.cend())
                if (const auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                    if (get_solver().get_sat_core().value(a0_a1_it->second) != utils::False && vars.insert(variable(a0_a1_it->second)).second)
                        add_resolver(new order_resolver(*this, a0_a1_it->second, *as[0], *as[1]));

            if (const auto a1_it = rr.leqs.find(as[1]); a1_it != rr.leqs.cend())
                if (const auto a1_a0_it = a1_it->second.find(as[0]); a1_a0_it != a1_it->second.cend())
                    if (get_solver().get_sat_core().value(a1_a0_it->second) != utils::False && vars.insert(variable(a1_a0_it->second)).second)
                        add_resolver(new order_resolver(*this, a1_a0_it->second, *as[1], *as[0]));
        }
        for (auto atm : overlapping_atoms)
            if (const auto atm_frbs = rr.frbs.find(atm); atm_frbs != rr.frbs.cend())
                for (const auto &atm_rr : atm_frbs->second)
                    if (get_solver().get_sat_core().value(atm_rr.second) != utils::False && vars.insert(variable(atm_rr.second)).second)
                        add_resolver(new forbid_resolver(*this, atm_rr.second, *atm, *atm_rr.first));
    }

    reusable_resource::rr_flaw::order_resolver::order_resolver(rr_flaw &flw, const semitone::lit &r, const atom &before, const atom &after) : resolver(flw, r, utils::rational::ZERO), before(before), after(after) {}

    json::json reusable_resource::rr_flaw::order_resolver::get_data() const noexcept { return {{"type", "order"}, {"before", get_id(before)}, {"after", get_id(after)}}; }

    reusable_resource::rr_flaw::forbid_resolver::forbid_resolver(rr_flaw &flw, const semitone::lit &r, atom &atm, riddle::item &itm) : resolver(flw, r, utils::rational::ZERO), atm(atm), itm(itm) {}

    json::json reusable_resource::rr_flaw::forbid_resolver::get_data() const noexcept { return {{"type", "forbid"}, {"forbid_atom", get_id(atm)}, {"forbid_item", get_id(itm)}}; }

    json::json reusable_resource::extract() const noexcept
    {
        json::json tls(json::json_type::array);
        // we partition atoms for each reusable-resource they might insist on..
        std::unordered_map<riddle::complex_item *, std::vector<atom *>> rr_instances;
        for (const auto &rr : get_instances())
            rr_instances.emplace(static_cast<riddle::complex_item *>(rr.operator->()), std::vector<atom *>());

        for (const auto &atm : atoms)
            if (get_solver().get_sat_core().value(atm->get_sigma()) == utils::True) // we filter out those atoms which are not strictly active..
            {
                const auto rr = atm->get(TAU_KW); // we get the reusable-resource..
                if (auto enum_scope = dynamic_cast<enum_item *>(rr.operator->()))
                { // the `tau` parameter is a variable..
                    for (const auto &rr_val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        rr_instances.at(static_cast<riddle::complex_item *>(rr_val)).emplace_back(atm);
                }
                else // the `tau` parameter is a constant..
                    rr_instances.at(static_cast<riddle::complex_item *>(rr.operator->())).emplace_back(atm);
            }

        for (const auto &[rr, atms] : rr_instances)
        {
            json::json tl;
            tl["id"] = get_id(*rr);
#ifdef COMPUTE_NAMES
            tl["name"] = get_solver().guess_name(*rr);
#endif
            tl["type"] = REUSABLE_RESOURCE_NAME;

            const auto c_capacity = get_solver().arith_value(rr->get(REUSABLE_RESOURCE_CAPACITY));
            tl["capacity"] = to_json(c_capacity);

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
                utils::inf_rational c_usage; // the concurrent resource usage..
                for (const auto &atm : overlapping_atoms)
                {
                    c_usage += get_solver().arith_value(atm->get(REUSABLE_RESOURCE_AMOUNT_NAME));
                    j_atms.push_back(get_id(*atm));
                }
                j_val["atoms"] = std::move(j_atms);
                j_val["usage"] = to_json(c_usage);
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