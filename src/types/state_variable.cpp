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
        std::unordered_map<const ratio::core::complex_item *, std::vector<ratio::core::atom *>> sv_instances;
        for (const auto &atm : atoms)
            if (get_solver().get_sat_core()->value(get_sigma(get_solver(), *atm)) == semitone::True) // we filter out those which are not strictly active..
            {
                const auto c_scope = atm->get(TAU_KW);
                if (auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope))
                {
                    for (const auto &val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        if (to_check.count(static_cast<const ratio::core::complex_item *>(val)))
                            sv_instances[static_cast<const ratio::core::complex_item *>(val)].emplace_back(atm);
                }
                else if (to_check.count(static_cast<ratio::core::complex_item *>(&*c_scope)))
                    sv_instances[static_cast<ratio::core::complex_item *>(&*c_scope)].emplace_back(atm);
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
                const auto start = get_core().arith_value(atm->get(RATIO_START));
                const auto end = get_core().arith_value(atm->get(RATIO_END));
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
                    for (const auto &as : semitone::combinations(std::vector<ratio::core::atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2))
                    { // state-variable MCSs are made of two atoms..
                        std::set<ratio::core::atom *> mcs(as.cbegin(), as.cend());
                        if (!sv_flaws.count(mcs))
                        { // we create a new state-variable flaw..
                            auto flw = std::make_unique<sv_flaw>(*this, mcs);
                            sv_flaws.insert({mcs, flw.get()});
                            store_flaw(std::move(flw)); // we store the flaw for retrieval when at root-level..
                        }

                        std::vector<std::pair<semitone::lit, double>> choices;
                        const auto a0_start = as[0]->get(RATIO_START);
                        const auto a0_end = as[0]->get(RATIO_END);
                        const auto a1_start = as[1]->get(RATIO_START);
                        const auto a1_end = as[1]->get(RATIO_END);

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

                        const auto a0_tau = as[0]->get(TAU_KW);
                        const auto a1_tau = as[1]->get(TAU_KW);
                        const auto a0_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a0_tau);
                        const auto a1_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a1_tau);
                        if (a0_tau_itm && a1_tau_itm)
                        { // we have two, perhaps singleton, enum variables..
                            const auto a0_vals = get_core().enum_value(*a0_tau_itm);
                            const auto a1_vals = get_core().enum_value(*a1_tau_itm);
                            if (a0_vals.size() > 1 && a1_vals.size() > 1)
                            { // we have two non-singleton variables..
                                for (const auto &plc : plcs.at({as[0], as[1]}))
                                    if (get_solver().get_sat_core()->value(plc.first) == semitone::Undefined)
                                        choices.emplace_back(plc.first, 1l - 2l / static_cast<double>(a0_vals.size() + a1_vals.size()));
                            }
                            else if (a0_vals.size() > 1)
                            { // only `a1_tau` is a singleton variable..
                                if (get_solver().get_sat_core()->value(get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item *>(a0_tau_itm)->get_var(), *a1_tau)) == semitone::Undefined)
                                    choices.emplace_back(!get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item *>(a0_tau_itm)->get_var(), *a1_tau), 1l - 1l / static_cast<double>(a0_vals.size()));
                            }
                            else if (a1_vals.size() > 1)
                            { // only `a0_tau` is a singleton variable..
                                if (get_solver().get_sat_core()->value(get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item *>(a1_tau_itm)->get_var(), *a0_tau)) == semitone::Undefined)
                                    choices.emplace_back(!get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item *>(a1_tau_itm)->get_var(), *a0_tau), 1l - 1l / static_cast<double>(a1_vals.size()));
                            }
                        }
                        else if (a0_tau_itm)
                        { // only `a1_tau` is a singleton variable..
                            if (const auto a0_vals = get_solver().enum_value(*a0_tau_itm); a0_vals.count(&*a1_tau))
                                if (get_solver().get_sat_core()->value(get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item *>(a0_tau_itm)->get_var(), *a1_tau)) == semitone::Undefined)
                                    choices.emplace_back(!get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item *>(a0_tau_itm)->get_var(), *a1_tau), 1l - 1l / static_cast<double>(a0_vals.size()));
                        }
                        else if (a1_tau_itm)
                        { // only `a0_tau` is a singleton variable..
                            if (const auto a1_vals = get_solver().enum_value(*a1_tau_itm); a1_vals.count(&*a0_tau))
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
            auto atm_expr = f.get_atom_expr();
            get_solver().get_interval().apply_rule(atm_expr);
            restore_ni();
        }

        // we store the variables for on-line flaw resolution..
        for (const auto &c_atm : atoms)
            store_variables(atm, *c_atm);

        // we store, for the atom, its atom listener..
        atoms.emplace_back(&atm);
        listeners.emplace_back(std::make_unique<sv_atom_listener>(*this, atm));

        // we filter out those atoms which are not strictly active..
        if (get_solver().get_sat_core()->value(get_sigma(get_solver(), atm)) == semitone::True)
        {
            const auto c_scope = atm.get(TAU_KW);
            if (const auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope))        // the `tau` parameter is a variable..
                for (const auto &val : get_solver().get_ov_theory().value(enum_scope->get_var())) // we check for all its allowed values..
                    to_check.insert(static_cast<const ratio::core::item *>(val));
            else // the `tau` parameter is a constant..
                to_check.insert(&*c_scope);
        }
    }

    void state_variable::store_variables(ratio::core::atom &atm0, ratio::core::atom &atm1)
    {
        const auto a0_start = atm0.get(RATIO_START);
        const auto a0_end = atm0.get(RATIO_END);
        const auto a1_start = atm1.get(RATIO_START);
        const auto a1_end = atm1.get(RATIO_END);

        const auto a0_tau = atm0.get(TAU_KW);
        const auto a1_tau = atm1.get(TAU_KW);
        const auto a0_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a0_tau);
        const auto a1_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a1_tau);
        if (a0_tau_itm && a1_tau_itm)
        { // we have two, perhaps singleton, enum variables..
            const auto a0_vals = get_solver().enum_value(*a0_tau_itm);
            const auto a1_vals = get_solver().enum_value(*a1_tau_itm);

            bool found = false;
            for (const auto &v0 : a0_vals)
                if (a1_vals.count(v0))
                { // the two atoms can affect the same resource..
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
                    if (a0_vals.size() > 1 && a1_vals.size() > 1) // we store a variable for forcing a0 in v0 and a1 not in v0..
                        plcs[{&atm0, &atm1}].emplace_back(get_solver().get_sat_core()->new_conj({get_solver().get_ov_theory().allows(a0_tau_itm->get_var(), *v0), !get_solver().get_ov_theory().allows(a1_tau_itm->get_var(), *v0)}), static_cast<const ratio::core::item *>(v0));
                }
            assert(!plcs.count({&atm0, &atm1}) || plcs.at({&atm0, &atm1}).size() > 1);
        }
        else if (a0_tau_itm)
        { // only `a1_tau` is a singleton variable..
            if (const auto a0_vals = get_solver().enum_value(*a0_tau_itm); a0_vals.count(&*a1_tau))
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
        { // only `a0_tau` is a singleton variable..
            if (const auto a1_vals = get_solver().enum_value(*a1_tau_itm); a1_vals.count(&*a0_tau))
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
            const auto c_scope = atm.get(TAU_KW);
            if (const auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope))           // the `tau` parameter is a variable..
                for (const auto &val : sv.get_solver().get_ov_theory().value(enum_scope->get_var())) // we check for all its allowed values..
                    sv.to_check.insert(static_cast<const ratio::core::item *>(val));
            else // the `tau` parameter is a constant..
                sv.to_check.insert(&*c_scope);
        }
    }

    state_variable::sv_flaw::sv_flaw(state_variable &sv, const std::set<ratio::core::atom *> &atms) : flaw(sv.get_solver(), smart_type::get_resolvers(sv.get_solver(), atms), {}), sv(sv), overlapping_atoms(atms) {}

    ORATIOSOLVER_EXPORT json::json state_variable::sv_flaw::get_data() const noexcept
    {
        json::json j_sv_f;
        j_sv_f["type"] = "sv-flaw";

        json::array j_atms;
        for (const auto &atm : overlapping_atoms)
            j_atms.push_back(get_id(*atm));
        j_sv_f["atoms"] = std::move(j_atms);

        return j_sv_f;
    }

    void state_variable::sv_flaw::compute_resolvers()
    {
        const auto cs = semitone::combinations(std::vector<ratio::core::atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2);
        for (const auto &as : cs)
        {
            if (const auto a0_it = sv.leqs.find(as[0]); a0_it != sv.leqs.cend())
                if (const auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                    if (get_solver().get_sat_core()->value(a0_a1_it->second) != semitone::False)
                        add_resolver(std::make_unique<order_resolver>(*this, a0_a1_it->second, *as[0], *as[1]));

            if (const auto a1_it = sv.leqs.find(as[1]); a1_it != sv.leqs.cend())
                if (const auto a1_a0_it = a1_it->second.find(as[0]); a1_a0_it != a1_it->second.cend())
                    if (get_solver().get_sat_core()->value(a1_a0_it->second) != semitone::False)
                        add_resolver(std::make_unique<order_resolver>(*this, a1_a0_it->second, *as[1], *as[0]));

            const auto a0_tau = as[0]->get(TAU_KW);
            const auto a1_tau = as[1]->get(TAU_KW);
            ratio::core::enum_item *a0_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a0_tau);
            ratio::core::enum_item *a1_tau_itm = dynamic_cast<ratio::core::enum_item *>(&*a1_tau);
            if (a0_tau_itm && a1_tau_itm)
            { // we have two, perhaps singleton, enum variables..
                const auto a0_vals = get_solver().enum_value(*a0_tau_itm);
                const auto a1_vals = get_solver().enum_value(*a1_tau_itm);
                if (a0_vals.size() > 1 && a1_vals.size() > 1)
                { // we have two non-singleton variables..
                    for (const auto &a0_a1_disp : sv.plcs.at({as[0], as[1]}))
                        if (get_solver().get_sat_core()->value(a0_a1_disp.first) == semitone::Undefined)
                            add_resolver(std::make_unique<place_resolver>(*this, a0_a1_disp.first, *as[0], *a0_a1_disp.second, *as[1]));
                }
                else if (a0_vals.size() > 1) // only `a1_tau` is a singleton variable..
                    add_resolver(std::make_unique<forbid_resolver>(*this, *as[0], *a1_tau));
                else if (a1_vals.size() > 1) // only `a0_tau` is a singleton variable..
                    add_resolver(std::make_unique<forbid_resolver>(*this, *as[1], *a0_tau));
            }
            else if (a0_tau_itm && get_solver().enum_value(*a0_tau_itm).size() > 1) // only `a1_tau` is a singleton variable..
                add_resolver(std::make_unique<forbid_resolver>(*this, *as[0], *a1_tau));
            else if (a1_tau_itm && get_solver().enum_value(*a1_tau_itm).size() > 1) // only `a0_tau` is a singleton variable..
                add_resolver(std::make_unique<forbid_resolver>(*this, *as[1], *a0_tau));
        }
    }

    state_variable::order_resolver::order_resolver(sv_flaw &flw, const semitone::lit &r, const ratio::core::atom &before, const ratio::core::atom &after) : resolver(r, semitone::rational::ZERO, flw), before(before), after(after) {}

    ORATIOSOLVER_EXPORT json::json state_variable::order_resolver::get_data() const noexcept
    {
        json::json j_r;
        j_r["type"] = "order";
        j_r["before_atom"] = get_id(before);
        j_r["after_atom"] = get_id(after);
        return j_r;
    }

    void state_variable::order_resolver::apply() {}

    state_variable::place_resolver::place_resolver(sv_flaw &flw, const semitone::lit &r, ratio::core::atom &plc_atm, const ratio::core::item &plc_itm, ratio::core::atom &frbd_atm) : resolver(r, semitone::rational::ZERO, flw), plc_atm(plc_atm), plc_itm(plc_itm), frbd_atm(frbd_atm) {}

    ORATIOSOLVER_EXPORT json::json state_variable::place_resolver::get_data() const noexcept
    {
        json::json j_r;
        j_r["type"] = "place";
        j_r["place_atom"] = get_id(plc_atm);
        j_r["forbid_atom"] = get_id(frbd_atm);
        return j_r;
    }

    void state_variable::place_resolver::apply() {}

    state_variable::forbid_resolver::forbid_resolver(sv_flaw &flw, ratio::core::atom &atm, ratio::core::item &itm) : resolver(semitone::rational::ZERO, flw), atm(atm), itm(itm) {}

    ORATIOSOLVER_EXPORT json::json state_variable::forbid_resolver::get_data() const noexcept
    {
        json::json j_r;
        j_r["type"] = "forbid";
        j_r["atom"] = get_id(atm);
        return j_r;
    }

    void state_variable::forbid_resolver::apply() { get_solver().get_sat_core()->new_clause({!get_rho(), !get_solver().get_ov_theory().allows(static_cast<ratio::core::enum_item &>(*atm.get(TAU_KW)).get_var(), itm)}); }

    json::json state_variable::extract() const noexcept
    {
        json::array tls;
        // we partition atoms for each state-variable they might insist on..
        std::unordered_map<const ratio::core::item *, std::vector<ratio::core::atom *>> sv_instances;
        for (auto &sv_instance : get_instances())
            sv_instances[&*sv_instance];
        for (const auto &atm : get_atoms())
            if (get_solver().get_sat_core()->value(get_sigma(get_solver(), *atm)) == semitone::True) // we filter out those which are not strictly active..
            {
                const auto c_scope = atm->get(TAU_KW);
                if (const auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope))
                    for (const auto &val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        sv_instances.at(static_cast<const ratio::core::item *>(val)).emplace_back(atm);
                else
                    sv_instances.at(static_cast<ratio::core::item *>(&*c_scope)).emplace_back(atm);
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
            std::map<semitone::inf_rational, std::set<ratio::core::atom *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<semitone::inf_rational, std::set<ratio::core::atom *>> ending_atoms;
            // all the pulses of the timeline..
            std::set<semitone::inf_rational> pulses;

            for (const auto &atm : atms)
            {
                const auto start = get_solver().ratio::core::core::arith_value(atm->get(RATIO_START));
                const auto end = get_solver().ratio::core::core::arith_value(atm->get(RATIO_END));
                starting_atoms[start].insert(atm);
                ending_atoms[end].insert(atm);
                pulses.insert(start);
                pulses.insert(end);
            }
            pulses.insert(get_solver().ratio::core::core::arith_value(get_solver().get("origin")));
            pulses.insert(get_solver().ratio::core::core::arith_value(get_solver().get("horizon")));

            std::set<ratio::core::atom *> overlapping_atoms;
            std::set<semitone::inf_rational>::iterator p = pulses.begin();
            if (const auto at_start_p = starting_atoms.find(*p); at_start_p != starting_atoms.cend())
                overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
            if (const auto at_end_p = ending_atoms.find(*p); at_end_p != ending_atoms.cend())
                for (const auto &a : at_end_p->second)
                    overlapping_atoms.erase(a);

            json::array j_vals;
            for (p = std::next(p); p != pulses.end(); ++p)
            {
                json::json j_val;
                j_val["from"] = to_json(*std::prev(p));
                j_val["to"] = to_json(*p);

                json::array j_atms;
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
} // namespace ratio::solver