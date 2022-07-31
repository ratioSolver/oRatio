#include "state_variable.h"
#include "solver.h"
#include "predicate.h"
#include "field.h"
#include "atom_flaw.h"
#include "combinations.h"

namespace ratio::solver
{
    state_variable::state_variable(solver &slv) : smart_type(slv, STATE_VARIABLE_NAME) { new_constructor(std::make_unique<sv_constructor>(*this)); }

    std::vector<std::vector<std::pair<semitone::lit, double>>> state_variable::get_current_incs()
    {
        std::vector<std::vector<std::pair<semitone::lit, double>>> incs;
        // we partition atoms for each state-variable they might insist on..
        std::unordered_map<const ratio::core::item *, std::vector<ratio::core::atom *>> sv_instances;
        for ([[maybe_unused]] const auto &[atm, atm_lstnr] : atoms)
            if (get_solver().get_sat_core()->value(get_sigma(get_solver(), *atm)) == semitone::True) // we filter out those which are not strictly active..
            {
                auto c_scope = atm->get(TAU_KW);
                if (auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope))
                {
                    for (const auto &val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        if (to_check.count(static_cast<const ratio::core::item *>(val)))
                            sv_instances[static_cast<const ratio::core::item *>(val)].emplace_back(atm);
                }
                else if (to_check.count(static_cast<ratio::core::item *>(&*c_scope)))
                    sv_instances[static_cast<ratio::core::item *>(&*c_scope)].emplace_back(atm);
            }

        // we detect inconsistencies for each of the instances..
        for ([[maybe_unused]] const auto &[sv, atms] : sv_instances)
        {
            // for each pulse, the atoms starting at that pulse..
            std::map<semitone::inf_rational, std::set<ratio::core::atom *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<semitone::inf_rational, std::set<ratio::core::atom *>> ending_atoms;
            // all the pulses of the timeline..
            std::set<semitone::inf_rational> pulses;

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

                if (overlapping_atoms.size() > 1) // we have a 'peak'..
                    for (const auto &as : combinations(std::vector<ratio::core::atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2))
                    { // state-variable MCSs are made of two atoms..
                        std::set<ratio::core::atom *> mcs(as.cbegin(), as.cend());
                        if (!sv_flaws.count(mcs))
                        { // we create a new state-variable flaw..
                            sv_flaw *flw = new sv_flaw(*this, mcs);
                            sv_flaws.insert({mcs, flw});
                            store_flaw(*flw); // we store the flaw for retrieval when at root-level..
                        }

                        std::vector<std::pair<semitone::lit, double>> choices;
                        auto a0_start = as[0]->get(RATIO_START);
                        auto a0_end = as[0]->get(RATIO_END);
                        auto a1_start = as[1]->get(RATIO_START);
                        auto a1_end = as[1]->get(RATIO_END);

                        if (auto a0_it = leqs.find(as[0]); a0_it != leqs.cend())
                            if (auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                                if (get_solver().get_sat_core()->value(a0_a1_it->second) != semitone::False)
                                {
#ifdef DL_TN
                                    const auto [min, max] = get_solver().get_rdl_theory().distance(a0_end->l, a1_start->l);
                                    const auto commit = is_infinite(min) || is_infinite(max) ? 0.5 : (std::min(static_cast<double>(max.get_rational()), 0.0) - std::min(static_cast<double>(min.get_rational()), 0.0)) / (static_cast<double>(max.get_rational()) - static_cast<double>(min.get_rational()));
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
                                    const auto [min, max] = get_solver().get_rdl_theory().distance(a1_end->l, a0_start->l);
                                    const auto commit = is_infinite(min) || is_infinite(max) ? 0.5 : (std::min(static_cast<double>(max.get_rational()), 0.0) - std::min(static_cast<double>(min.get_rational()), 0.0)) / (static_cast<double>(max.get_rational()) - static_cast<double>(min.get_rational()));
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
                            auto a0_vals = get_solver().enum_value(*a0_tau_itm);
                            auto a1_vals = get_solver().enum_value(*a1_tau_itm);
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
                        incs.emplace_back(choices);
                    }
            }
        }
        return incs;
    }

    void state_variable::new_predicate(ratio::core::predicate &pred) noexcept
    {
        // each state-variable predicate is also an interval-predicate..
        new_supertype(pred, get_solver().get_interval());
        // each state-variable predicate has a tau parameter indicating on which state-variables the atoms insist on..
        new_field(pred, std::make_unique<ratio::core::field>(static_cast<type &>(pred.get_scope()), TAU_KW));
    }

    void state_variable::new_atom_flaw(atom_flaw &f)
    {
        auto &atm = f.get_atom();
        if (f.is_fact)
        { // we apply interval-predicate whenever the fact becomes active..
            set_ni(semitone::lit(get_sigma(get_solver(), atm)));
            get_solver().get_interval().apply_rule(atm);
            restore_ni();
        }

        // we store the variables for on-line flaw resolution..
        for (const auto &c_atm : atoms)
            store_variables(atm, *c_atm.first);

        // we store, for the atom, its atom listener..
        atoms.emplace_back(&atm, new sv_atom_listener(*this, atm));

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

    void state_variable::store_variables(ratio::core::atom &atm0, ratio::core::atom &atm1)
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
            auto a0_vals = get_core().enum_value(*a0_tau_itm);
            auto a1_vals = get_core().enum_value(*a1_tau_itm);

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
        { // the two atoms are on the same state-variable: we store the ordering variables..
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

    state_variable::sv_constructor::sv_constructor(state_variable &sv) : ratio::core::constructor(sv, {}, {}, {}, {}) {}

    state_variable::sv_atom_listener::sv_atom_listener(state_variable &sv, ratio::core::atom &atm) : atom_listener(atm), sv(sv) { something_changed(); }

    void state_variable::sv_atom_listener::something_changed()
    {
        // we filter out those atoms which are not strictly active..
        if (sv.get_solver().get_sat_core()->value(get_sigma(sv.get_solver(), atm)) == semitone::True)
        {
            auto c_scope = atm.get(TAU_KW);
            if (auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope)) // the 'tau' parameter is a variable..
                for (const auto &val : sv.get_solver().get_ov_theory().value(enum_scope->get_var()))    // we check for all its allowed values..
                    sv.to_check.insert(static_cast<const ratio::core::item *>(val));
            else // the 'tau' parameter is a constant..
                sv.to_check.insert(&*c_scope);
        }
    }

    state_variable::sv_flaw::sv_flaw(state_variable &sv, const std::set<ratio::core::atom *> &atms) : flaw(sv.get_solver(), smart_type::get_resolvers(sv.get_solver(), atms), {}), sv(sv), overlapping_atoms(atms) {}

    void state_variable::sv_flaw::compute_resolvers()
    {
        const auto cs = combinations(std::vector<ratio::core::atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2);
        for (const auto &as : cs)
        {
            if (auto a0_it = sv.leqs.find(as[0]); a0_it != sv.leqs.cend())
                if (auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                    if (get_solver().get_sat_core()->value(a0_a1_it->second) != semitone::False)
                        add_resolver(std::make_unique<order_resolver>(*this, a0_a1_it->second, *as[0], *as[1]));

            if (auto a1_it = sv.leqs.find(as[1]); a1_it != sv.leqs.cend())
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
            else if (auto a0_a1_it = sv.plcs.find({as[0], as[1]}); a0_a1_it != sv.plcs.cend())
                for (const auto &a0_a1_disp : a0_a1_it->second)
                    if (get_solver().get_sat_core()->value(a0_a1_disp.first) != semitone::False)
                        add_resolver(std::make_unique<place_resolver>(*this, a0_a1_disp.first, *as[0], *a0_a1_disp.second, *as[1]));
        }
    }

    state_variable::order_resolver::order_resolver(sv_flaw &flw, const semitone::lit &r, const ratio::core::atom &before, const ratio::core::atom &after) : resolver(r, semitone::rational::ZERO, flw), before(before), after(after) {}

    void state_variable::order_resolver::apply() {}

    state_variable::place_resolver::place_resolver(sv_flaw &flw, const semitone::lit &r, ratio::core::atom &plc_atm, const ratio::core::item &plc_itm, ratio::core::atom &frbd_atm) : resolver(r, semitone::rational::ZERO, flw), plc_atm(plc_atm), plc_itm(plc_itm), frbd_atm(frbd_atm) {}

    void state_variable::place_resolver::apply() {}

    state_variable::forbid_resolver::forbid_resolver(sv_flaw &flw, ratio::core::atom &atm, ratio::core::item &itm) : resolver(semitone::rational::ZERO, flw), atm(atm), itm(itm) {}

    void state_variable::forbid_resolver::apply() { get_solver().get_sat_core()->new_clause({!get_rho(), !get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item &>(*atm.get(TAU_KW)).get_var(), itm)}); }
} // namespace ratio::solver