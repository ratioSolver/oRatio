#include "reusable_resource.h"
#include "solver.h"
#include "item.h"
#include "field.h"
#include "atom_flaw.h"
#include "combinations.h"
#include "parser.h"
#include <stdexcept>
#include <list>

namespace ratio::solver
{
    reusable_resource::reusable_resource(solver &slv) : smart_type(slv, REUSABLE_RESOURCE_NAME)
    {
        // we add the 'capacity' field..
        new_field(*this, std::make_unique<ratio::core::field>(slv.get_real_type(), REUSABLE_RESOURCE_CAPACITY));

        // we add a constructor..
        std::vector<ratio::core::field_ptr> ctr_args;
        ctr_args.emplace_back(std::make_unique<ratio::core::field>(get_core().get_real_type(), REUSABLE_RESOURCE_CAPACITY));

        ctr_ins.emplace_back(riddle::id_token(0, 0, 0, 0, REUSABLE_RESOURCE_CAPACITY));

        std::vector<std::unique_ptr<const riddle::ast::expression>> i_c;
        i_c.emplace_back(std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::id_expression *>(new ratio::core::id_expression({riddle::id_token(0, 0, 0, 0, REUSABLE_RESOURCE_CAPACITY)}))));
        ctr_ivs.emplace_back(std::move(i_c));

        auto l_c_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::id_expression *>(new ratio::core::id_expression(std::vector<riddle::id_token>({riddle::id_token(0, 0, 0, 0, REUSABLE_RESOURCE_CAPACITY)}))));
        auto r_c_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::real_literal_expression *>(new ratio::core::real_literal_expression(riddle::real_token(0, 0, 0, 0, semitone::rational::ZERO))));
        auto c_ge_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::geq_expression *>(new ratio::core::geq_expression(std::move(l_c_xpr), std::move(r_c_xpr))));
        auto c_stmnt = std::unique_ptr<const riddle::ast::statement>(static_cast<const riddle::ast::expression_statement *>(new ratio::core::expression_statement(std::move(c_ge_xpr))));
        ctr_stmnts.emplace_back(std::move(c_stmnt));

        new_constructor(std::make_unique<rr_constructor>(*this, std::move(ctr_args), ctr_ins, ctr_ivs, ctr_stmnts));

        auto l_u_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::id_expression *>(new ratio::core::id_expression(std::vector<riddle::id_token>({riddle::id_token(0, 0, 0, 0, REUSABLE_RESOURCE_USE_AMOUNT_NAME)}))));
        auto r_u_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::real_literal_expression *>(new ratio::core::real_literal_expression(riddle::real_token(0, 0, 0, 0, semitone::rational::ZERO))));
        auto u_ge_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::geq_expression *>(new ratio::core::geq_expression(std::move(l_u_xpr), std::move(r_u_xpr))));
        auto u_stmnt = std::unique_ptr<const riddle::ast::statement>(static_cast<const riddle::ast::expression_statement *>(new ratio::core::expression_statement(std::move(u_ge_xpr))));
        pred_stmnts.emplace_back(std::move(u_stmnt));

        // we add the 'Use' predicate, without notifying neither the resource nor its supertypes..
        std::vector<ratio::core::field_ptr> cons_pred_args;
        cons_pred_args.push_back(std::make_unique<ratio::core::field>(get_core().get_real_type(), REUSABLE_RESOURCE_USE_AMOUNT_NAME));
        auto up = std::make_unique<use_predicate>(*this, std::move(cons_pred_args), pred_stmnts);
        u_pred = up.get();
        ratio::core::type::new_predicate(std::move(up));
    }

    std::vector<std::vector<std::pair<semitone::lit, double>>> reusable_resource::get_current_incs()
    {
        std::vector<std::vector<std::pair<semitone::lit, double>>> incs;
        // we partition atoms for each reusable-resource they might insist on..
        std::unordered_map<ratio::core::complex_item *, std::vector<ratio::core::atom *>> rr_instances;
        for ([[maybe_unused]] const auto &[atm, atm_lstnr] : atoms)
            if (get_solver().get_sat_core()->value(get_sigma(get_solver(), *atm)) == semitone::True) // we filter out those which are not strictly active..
            {
                auto c_scope = atm->get(TAU_KW);
                if (auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope))
                {
                    for (const auto &val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        if (to_check.count(static_cast<const ratio::core::complex_item *>(val)))
                            rr_instances[static_cast<ratio::core::complex_item *>(val)].emplace_back(atm);
                }
                else if (to_check.count(static_cast<ratio::core::item *>(&*c_scope)))
                    rr_instances[static_cast<ratio::core::complex_item *>(&*c_scope)].emplace_back(atm);
            }

        // we detect inconsistencies for each of the instances..
        for (const auto &[rr, atms] : rr_instances)
        {
            // for each pulse, the atoms starting at that pulse..
            std::map<semitone::inf_rational, std::set<ratio::core::atom *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<semitone::inf_rational, std::set<ratio::core::atom *>> ending_atoms;
            // all the pulses of the timeline..
            std::set<semitone::inf_rational> pulses;
            // the resource capacity..
            auto c_capacity = get_core().arith_value(rr->get(REUSABLE_RESOURCE_CAPACITY));

            for (const auto &atm : atms)
            {
                auto start = get_core().arith_value(atm->get(RATIO_START));
                auto end = get_core().arith_value(atm->get(RATIO_END));
                starting_atoms[start].insert(atm);
                ending_atoms[end].insert(atm);
                pulses.insert(start);
                pulses.insert(end);
            }

            std::set<ratio::core::atom *> overlapping_atoms;
            for (const auto &p : pulses)
            {
                if (const auto at_start_p = starting_atoms.find(p); at_start_p != starting_atoms.cend())
                    overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
                if (const auto at_end_p = ending_atoms.find(p); at_end_p != ending_atoms.cend())
                    for (const auto &a : at_end_p->second)
                        overlapping_atoms.erase(a);

                semitone::inf_rational c_usage; // the concurrent resource usage..
                for (const auto &a : overlapping_atoms)
                    c_usage += get_core().arith_value(a->get(REUSABLE_RESOURCE_USE_AMOUNT_NAME));

                if (c_usage > c_capacity) // we have a 'peak'..
                {
                    // we extract minimal conflict sets (MCSs)..
                    // we sort the overlapping atoms, according to their resource usage, in descending order..
                    std::vector<ratio::core::atom *> inc_atoms(overlapping_atoms.cbegin(), overlapping_atoms.cend());
                    std::sort(inc_atoms.begin(), inc_atoms.end(), [this](const auto &atm0, const auto &atm1)
                              { return get_core().arith_value(atm0->get(REUSABLE_RESOURCE_USE_AMOUNT_NAME)) > get_core().arith_value(atm1->get(REUSABLE_RESOURCE_USE_AMOUNT_NAME)); });

                    semitone::inf_rational mcs_usage;     // the concurrent resource usage of the mcs..
                    std::list<ratio::core::atom *> c_mcs; // the current mcs..
                    auto mcs_begin = inc_atoms.cbegin();
                    auto mcs_end = inc_atoms.cbegin();
                    while (mcs_end != inc_atoms.cend())
                    {
                        // we increase the size of the current mcs..
                        while (mcs_usage <= c_capacity && mcs_end != inc_atoms.cend())
                        {
                            c_mcs.emplace_back(*mcs_end);
                            mcs_usage += get_core().arith_value((*mcs_end)->get(REUSABLE_RESOURCE_USE_AMOUNT_NAME));
                            ++mcs_end;
                        }

                        if (mcs_usage > c_capacity)
                        { // we have a new mcs..
                            std::set<ratio::core::atom *> mcs(c_mcs.cbegin(), c_mcs.cend());
                            if (!rr_flaws.count(mcs))
                            { // we create a new reusable-resource flaw..
                                rr_flaw *flw = new rr_flaw(*this, mcs);
                                rr_flaws.insert({mcs, flw});
                                store_flaw(*flw); // we store the flaw for retrieval when at root-level..
                            }

                            std::vector<std::pair<semitone::lit, double>> choices;
                            for (const auto &as : combinations(std::vector<ratio::core::atom *>(c_mcs.cbegin(), c_mcs.cend()), 2))
                            {
                                auto a0_start = as[0]->get(RATIO_START);
                                auto a0_end = as[0]->get(RATIO_END);
                                auto a1_start = as[1]->get(RATIO_START);
                                auto a1_end = as[1]->get(RATIO_END);

                                if (auto a0_it = leqs.find(as[0]); a0_it != leqs.cend())
                                    if (auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                                        if (get_solver().get_sat_core()->value(a0_a1_it->second) != semitone::False)
                                        {
#ifdef DL_TN
                                            const auto [min, max] = get_solver().get_rdl_theory().distance(static_cast<ratio::core::arith_item &>(*a0_end).get_value(), static_cast<ratio::core::arith_item &>(*a1_start).get_value());
                                            const auto commit = is_infinite(min) || is_infinite(max) ? 0.5 : (std::min(to_double(max.get_rational()), 0.0) - std::min(to_double(min.get_rational()), 0.0)) / (to_double(max.get_rational()) - to_double(min.get_rational()));
#elif LA_TN
                                            const auto work = (get_core().arith_value(a1_end).get_rational() - get_core().arith_value(a1_start).get_rational()) * (get_core().arith_value(a0_end).get_rational() - get_core().arith_value(a1_start).get_rational());
                                            const auto commit = work == semitone::rational::ZERO ? -std::numeric_limits<double>::max() : 1l - 1l / (static_cast<double>(work.numerator()) / work.denominator());
#endif
                                            choices.emplace_back(a0_a1_it->second, commit);
                                        }

                                if (auto a1_it = leqs.find(as[1]); a1_it != leqs.cend())
                                    if (auto a1_a0_it = a1_it->second.find(as[0]); a1_a0_it != a1_it->second.cend())
                                        if (get_solver().get_sat_core()->value(a1_a0_it->second) != semitone::False)
                                        {
#ifdef DL_TN
                                            const auto [min, max] = get_solver().get_rdl_theory().distance(static_cast<ratio::core::arith_item &>(*a1_end).get_value(), static_cast<ratio::core::arith_item &>(*a0_start).get_value());
                                            const auto commit = is_infinite(min) || is_infinite(max) ? 0.5 : (std::min(to_double(max.get_rational()), 0.0) - std::min(to_double(min.get_rational()), 0.0)) / (to_double(max.get_rational()) - to_double(min.get_rational()));
#elif LA_TN
                                            const auto work = (get_core().arith_value(a0_end).get_rational() - get_core().arith_value(a0_start).get_rational()) * (get_core().arith_value(a1_end).get_rational() - get_core().arith_value(a0_start).get_rational());
                                            const auto commit = work == semitone::rational::ZERO ? -std::numeric_limits<double>::max() : 1l - 1l / (static_cast<double>(work.numerator()) / work.denominator());
#endif
                                            choices.emplace_back(a1_a0_it->second, commit);
                                        }

                                auto a0_tau = as[0]->get(TAU_KW);
                                auto a1_tau = as[1]->get(TAU_KW);
                                auto a0_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a0_tau);
                                auto a1_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a1_tau);
                                if (a0_tau_itm && a1_tau_itm)
                                { // we have two non-singleton variables..
                                    auto a0_vals = get_core().enum_value(*a0_tau_itm);
                                    auto a1_vals = get_core().enum_value(*a1_tau_itm);
                                    for (const auto &plc : plcs.at({as[0], as[1]}))
                                        if (get_solver().get_sat_core()->value(plc.first) == semitone::Undefined)
                                            choices.emplace_back(plc.first, 1l - 2l / static_cast<double>(a0_vals.size() + a1_vals.size()));
                                }
                                else if (a0_tau_itm)
                                { // only 'a1_tau' is a singleton variable..
                                    if (auto a0_vals = get_solver().enum_value(*a0_tau_itm); a0_vals.count(&*a1_tau))
                                        if (get_solver().get_sat_core()->value(get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item *>(a0_tau_itm)->get_var(), *a1_tau)) == semitone::Undefined)
                                            choices.emplace_back(!get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item *>(a0_tau_itm)->get_var(), *a1_tau), 1l - 1l / static_cast<double>(a0_vals.size()));
                                }
                                else if (a1_tau_itm)
                                { // only 'a0_tau' is a singleton variable..
                                    if (auto a1_vals = get_solver().enum_value(*a1_tau_itm); a1_vals.count(&*a0_tau))
                                        if (get_solver().get_sat_core()->value(get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item *>(a1_tau_itm)->get_var(), *a0_tau)) == semitone::Undefined)
                                            choices.emplace_back(!get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item *>(a1_tau_itm)->get_var(), *a0_tau), 1l - 1l / static_cast<double>(a1_vals.size()));
                                }
                            }
                            incs.emplace_back(choices);

                            // we decrease the size of the mcs..
                            mcs_usage -= get_core().arith_value(c_mcs.front()->get(REUSABLE_RESOURCE_USE_AMOUNT_NAME));
                            assert(mcs_usage <= c_capacity);
                            c_mcs.pop_front();
                            ++mcs_begin;
                        }
                    }
                }
            }
        }
        return incs;
    }

    void reusable_resource::new_predicate(ratio::core::predicate &pred) noexcept
    {
        assert(get_solver().is_interval(pred));
        assert(u_pred->is_assignable_from(pred));
        // each reusable-resource predicate has a tau parameter indicating on which resource the atoms insist on..
        new_field(pred, std::make_unique<ratio::core::field>(static_cast<type &>(pred.get_scope()), TAU_KW));
    }

    void reusable_resource::new_atom_flaw(atom_flaw &f)
    {
        auto &atm = f.get_atom();
        if (f.is_fact)
        { // we apply use-predicate whenever the fact becomes active..
            set_ni(semitone::lit(get_sigma(get_solver(), atm)));
            u_pred->apply_rule(atm);
            restore_ni();
        }

        // we avoid unification..
        if (!get_solver().get_sat_core()->new_clause({!f.get_phi(), semitone::lit(get_sigma(get_solver(), atm))}))
            throw ratio::core::unsolvable_exception();

        // we store the variables for on-line flaw resolution..
        for (const auto &c_atm : atoms)
            store_variables(atm, *c_atm.first);

        // we store, for the atom, its atom listener..
        atoms.emplace_back(&atm, new rr_atom_listener(*this, atm));

        // we filter out those atoms which are not strictly active..
        if (get_solver().get_sat_core()->value(get_sigma(get_solver(), atm)) == semitone::True)
        {
            auto c_scope = atm.get(TAU_KW);
            if (auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope))              // the 'tau' parameter is a variable..
                for (const auto &val : get_solver().get_ov_theory().value(enum_scope->get_var())) // we check for all its allowed values..
                    to_check.insert(static_cast<const ratio::core::item *>(val));
            else // the 'tau' parameter is a constant..
                to_check.insert(&*c_scope);
        }
    }

    void reusable_resource::store_variables(ratio::core::atom &atm0, ratio::core::atom &atm1)
    {
        auto a0_start = atm0.get(RATIO_START);
        auto a0_end = atm0.get(RATIO_END);
        auto a1_start = atm1.get(RATIO_START);
        auto a1_end = atm1.get(RATIO_END);

        auto a0_tau = atm0.get(TAU_KW);
        auto a1_tau = atm1.get(TAU_KW);
        auto a0_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a0_tau);
        auto a1_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a1_tau);
        if (a0_tau_itm && a1_tau_itm)
        { // we have two non-singleton variables..
            auto a0_vals = get_solver().enum_value(*a0_tau_itm);
            auto a1_vals = get_solver().enum_value(*a1_tau_itm);

            bool found = false;
            for (const auto &v0 : a0_vals)
                if (a1_vals.count(v0))
                {
                    if (!found)
                    { // we store the ordering variables..
#ifdef DL_TN
                        leqs[&atm0][&atm1] = get_solver().get_rdl_theory().new_leq(static_cast<ratio::core::arith_item &>(*a0_end).get_value(), static_cast<ratio::core::arith_item &>(*a1_start).get_value());
                        leqs[&atm1][&atm0] = get_solver().get_rdl_theory().new_leq(static_cast<ratio::core::arith_item &>(*a1_end).get_value(), static_cast<ratio::core::arith_item &>(*a0_start).get_value());
#elif LA_TN
                        leqs[&atm0][&atm1] = get_solver().get_lra_theory().new_leq(static_cast<ratio::core::arith_item &>(*a0_end).get_value(), static_cast<ratio::core::arith_item &>(*a1_start).get_value());
                        leqs[&atm1][&atm0] = get_solver().get_lra_theory().new_leq(static_cast<ratio::core::arith_item &>(*a1_end).get_value(), static_cast<ratio::core::arith_item &>(*a0_start).get_value());
                        // we boost propagation..
                        [[maybe_unused]] bool nc = get_solver().get_sat_core()->new_clause({!leqs[&atm0][&atm1], !leqs[&atm1][&atm0]});
                        assert(nc);
#endif
                        found = true;
                    }
                    plcs[{&atm0, &atm1}].emplace_back(get_solver().get_sat_core()->new_conj({get_solver().get_ov_theory().allows(a0_tau_itm->get_var(), *v0), !get_solver().get_ov_theory().allows(a1_tau_itm->get_var(), *v0)}), static_cast<const ratio::core::item *>(v0));
                }
        }
        else if (a0_tau_itm)
        { // only 'a1_tau' is a singleton variable..
            if (auto a0_vals = get_solver().enum_value(*a0_tau_itm); a0_vals.count(&*a1_tau))
            { // we store the ordering variables..
#ifdef DL_TN
                leqs[&atm0][&atm1] = get_solver().get_rdl_theory().new_leq(static_cast<ratio::core::arith_item &>(*a0_end).get_value(), static_cast<ratio::core::arith_item &>(*a1_start).get_value());
                leqs[&atm1][&atm0] = get_solver().get_rdl_theory().new_leq(static_cast<ratio::core::arith_item &>(*a1_end).get_value(), static_cast<ratio::core::arith_item &>(*a0_start).get_value());
#elif LA_TN
                leqs[&atm0][&atm1] = get_solver().get_lra_theory().new_leq(static_cast<ratio::core::arith_item &>(*a0_end).get_value(), static_cast<ratio::core::arith_item &>(*a1_start).get_value());
                leqs[&atm1][&atm0] = get_solver().get_lra_theory().new_leq(static_cast<ratio::core::arith_item &>(*a1_end).get_value(), static_cast<ratio::core::arith_item &>(*a0_start).get_value());
                // we boost propagation..
                [[maybe_unused]] bool nc = get_solver().get_sat_core()->new_clause({!leqs[&atm0][&atm1], !leqs[&atm1][&atm0]});
                assert(nc);
#endif
            }
        }
        else if (a1_tau_itm)
        { // only 'a0_tau' is a singleton variable..
            if (auto a1_vals = get_solver().enum_value(*a1_tau_itm); a1_vals.count(&*a0_tau))
            { // we store the ordering variables..
#ifdef DL_TN
                leqs[&atm0][&atm1] = get_solver().get_rdl_theory().new_leq(static_cast<ratio::core::arith_item &>(*a0_end).get_value(), static_cast<ratio::core::arith_item &>(*a1_start).get_value());
                leqs[&atm1][&atm0] = get_solver().get_rdl_theory().new_leq(static_cast<ratio::core::arith_item &>(*a1_end).get_value(), static_cast<ratio::core::arith_item &>(*a0_start).get_value());
#elif LA_TN
                leqs[&atm0][&atm1] = get_solver().get_lra_theory().new_leq(static_cast<ratio::core::arith_item &>(*a0_end).get_value(), static_cast<ratio::core::arith_item &>(*a1_start).get_value());
                leqs[&atm1][&atm0] = get_solver().get_lra_theory().new_leq(static_cast<ratio::core::arith_item &>(*a1_end).get_value(), static_cast<ratio::core::arith_item &>(*a0_start).get_value());
                // we boost propagation..
                [[maybe_unused]] bool nc = get_solver().get_sat_core()->new_clause({!leqs[&atm0][&atm1], !leqs[&atm1][&atm0]});
                assert(nc);
#endif
            }
        }
        else if (&*a0_tau == &*a1_tau)
        { // the two atoms are on the same reusable-resource: we store the ordering variables..
#ifdef DL_TN
            leqs[&atm0][&atm1] = get_solver().get_rdl_theory().new_leq(static_cast<ratio::core::arith_item &>(*a0_end).get_value(), static_cast<ratio::core::arith_item &>(*a1_start).get_value());
            leqs[&atm1][&atm0] = get_solver().get_rdl_theory().new_leq(static_cast<ratio::core::arith_item &>(*a1_end).get_value(), static_cast<ratio::core::arith_item &>(*a0_start).get_value());
#elif LA_TN
            leqs[&atm0][&atm1] = get_solver().get_lra_theory().new_leq(static_cast<ratio::core::arith_item &>(*a0_end).get_value(), static_cast<ratio::core::arith_item &>(*a1_start).get_value());
            leqs[&atm1][&atm0] = get_solver().get_lra_theory().new_leq(static_cast<ratio::core::arith_item &>(*a1_end).get_value(), static_cast<ratio::core::arith_item &>(*a0_start).get_value());
            // we boost propagation..
            [[maybe_unused]] bool nc = get_solver().get_sat_core()->new_clause({!leqs[&atm0][&atm1], !leqs[&atm1][&atm0]});
            assert(nc);
#endif
        }
    }

    reusable_resource::rr_constructor::rr_constructor(reusable_resource &rr, std::vector<ratio::core::field_ptr> args, const std::vector<riddle::id_token> &ins, const std::vector<std::vector<std::unique_ptr<const riddle::ast::expression>>> &ivs, const std::vector<std::unique_ptr<const riddle::ast::statement>> &stmnts) : constructor(rr, std::move(args), ins, ivs, stmnts) {}

    reusable_resource::use_predicate::use_predicate(reusable_resource &rr, std::vector<ratio::core::field_ptr> args, const std::vector<std::unique_ptr<const riddle::ast::statement>> &stmnts) : predicate(rr, REUSABLE_RESOURCE_USE_PREDICATE_NAME, std::move(args), stmnts) { new_supertype(rr.get_core().get_predicate(RATIO_INTERVAL)); }

    reusable_resource::rr_atom_listener::rr_atom_listener(reusable_resource &rr, ratio::core::atom &atm) : atom_listener(atm), rr(rr) {}

    void reusable_resource::rr_atom_listener::something_changed()
    {
        // we filter out those atoms which are not strictly active..
        if (rr.get_solver().get_sat_core()->value(get_sigma(rr.get_solver(), atm)) == semitone::True)
        {
            auto c_scope = atm.get(TAU_KW);
            if (auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope))                 // the 'tau' parameter is a variable..
                for (const auto &val : rr.get_solver().get_ov_theory().value(enum_scope->get_var())) // we check for all its allowed values..
                    rr.to_check.insert(static_cast<const ratio::core::item *>(val));
            else // the 'tau' parameter is a constant..
                rr.to_check.insert(&*c_scope);
        }
    }

    reusable_resource::rr_flaw::rr_flaw(reusable_resource &rr, const std::set<ratio::core::atom *> &atms) : flaw(rr.get_solver(), smart_type::get_resolvers(rr.get_solver(), atms), {}), rr(rr), overlapping_atoms(atms) {}

    void reusable_resource::rr_flaw::compute_resolvers()
    {
        const auto cs = combinations(std::vector<ratio::core::atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2);
        for (const auto &as : cs)
        {
            if (auto a0_it = rr.leqs.find(as[0]); a0_it != rr.leqs.cend())
                if (auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                    if (get_solver().get_sat_core()->value(a0_a1_it->second) != semitone::False)
                        add_resolver(std::make_unique<order_resolver>(*this, a0_a1_it->second, *as[0], *as[1]));

            if (auto a1_it = rr.leqs.find(as[1]); a1_it != rr.leqs.cend())
                if (auto a1_a0_it = a1_it->second.find(as[0]); a1_a0_it != a1_it->second.cend())
                    if (get_solver().get_sat_core()->value(a1_a0_it->second) != semitone::False)
                        add_resolver(std::make_unique<order_resolver>(*this, a1_a0_it->second, *as[1], *as[0]));

            auto a0_tau = as[0]->get(TAU_KW);
            auto a1_tau = as[1]->get(TAU_KW);
            ratio::core::enum_item *a0_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a0_tau);
            ratio::core::enum_item *a1_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a1_tau);
            if (a0_tau_itm && !a1_tau_itm)
                add_resolver(std::make_unique<forbid_resolver>(*this, *as[0], *a1_tau));
            else if (!a0_tau_itm && a1_tau_itm)
                add_resolver(std::make_unique<forbid_resolver>(*this, *as[1], *a0_tau));
            else if (auto a0_a1_it = rr.plcs.find({as[0], as[1]}); a0_a1_it != rr.plcs.cend())
                for (const auto &a0_a1_disp : a0_a1_it->second)
                    if (get_solver().get_sat_core()->value(a0_a1_disp.first) != semitone::False)
                        add_resolver(std::make_unique<place_resolver>(*this, a0_a1_disp.first, *as[0], *a0_a1_disp.second, *as[1]));
        }
    }

    reusable_resource::order_resolver::order_resolver(rr_flaw &flw, const semitone::lit &r, const ratio::core::atom &before, const ratio::core::atom &after) : resolver(r, semitone::rational::ZERO, flw), before(before), after(after) {}

    void reusable_resource::order_resolver::apply() {}

    reusable_resource::place_resolver::place_resolver(rr_flaw &flw, const semitone::lit &r, ratio::core::atom &plc_atm, const ratio::core::item &plc_itm, ratio::core::atom &frbd_atm) : resolver(r, semitone::rational::ZERO, flw), plc_atm(plc_atm), plc_itm(plc_itm), frbd_atm(frbd_atm) {}

    void reusable_resource::place_resolver::apply() {}

    reusable_resource::forbid_resolver::forbid_resolver(rr_flaw &flw, ratio::core::atom &atm, ratio::core::item &itm) : resolver(semitone::rational::ZERO, flw), atm(atm), itm(itm) {}

    void reusable_resource::forbid_resolver::apply() { get_solver().get_sat_core()->new_clause({!get_rho(), !get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item &>(*atm.get(TAU_KW)).get_var(), itm)}); }
} // namespace ratio::solver