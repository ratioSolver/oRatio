#include "state_variable.hpp"
#include "graph.hpp"
#include "combinations.hpp"
#include "logging.hpp"
#ifdef ENABLE_API
#include "solver_api.hpp"
#endif
#include <unordered_set>
#include <cassert>

namespace ratio
{
    state_variable::state_variable(solver &slv) : smart_type(slv, STATE_VARIABLE_TYPE_NAME) { add_constructor(std::make_unique<riddle::constructor>(*this, std::vector<std::unique_ptr<riddle::field>>{}, std::vector<riddle::init_element>{}, std::vector<std::unique_ptr<riddle::statement>>{})); }

    void state_variable::new_predicate(riddle::predicate &pred) { add_parent(pred, get_predicate(INTERVAL_PREDICATE_NAME)->get()); }

    std::vector<std::vector<std::pair<utils::lit, double>>> state_variable::get_current_incs() noexcept
    {
        std::vector<std::vector<std::pair<utils::lit, double>>> incs; // the inconsistencies..
        // we assign the atoms to the state-variables that need to be checked..
        std::unordered_map<const riddle::component *, std::vector<atom *>> sv_instances;
        for (const auto &atm : atoms)
            if (get_solver().get_sat().value(atm.get().get_sigma()) == utils::True)
            { // the atom is active..
                const auto tau = atm.get().get(riddle::TAU_NAME);
                if (is_enum(*tau))
                { // the `tau` parameter is a variable..
                    for (const auto &c_sv : get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
                        if (to_check.find(static_cast<const riddle::item *>(&c_sv.get())) != to_check.end())
                            sv_instances[static_cast<const riddle::component *>(&c_sv.get())].push_back(&atm.get());
                }
                else if (to_check.find(tau.get()) != to_check.end()) // the `tau` parameter is a constant..
                    sv_instances[static_cast<const riddle::component *>(tau.get())].push_back(&atm.get());
            }

        // we detect inconsistencies for each of the state-variable instances..
        for ([[maybe_unused]] const auto &[sv, atms] : sv_instances)
        {
            // for each pulse, the atoms starting at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> ending_atoms;
            // all the pulses of the timeline, sorted..
            std::set<utils::inf_rational> pulses;

            for (const auto &atm : atms)
            {
                const auto start = get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(atm->get(START_NAME)));
                const auto end = get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(atm->get(END_NAME)));
                starting_atoms[start].insert(atm);
                ending_atoms[end].insert(atm);
                pulses.insert(start);
                pulses.insert(end);
            }

            // we scroll through the timeline looking for inconsistencies..
            bool has_conflict = false;
            std::set<atom *> overlapping_atoms;
            for (const auto &p : pulses)
            {
                // we check if there are atoms that start at `p`, and if there are, we add them to the set of overlapping atoms..
                if (const auto at_start_p = starting_atoms.find(p); at_start_p != starting_atoms.cend())
                    overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
                // we check if there are atoms that end at `p`, and if there are, we remove them from the set of overlapping atoms..
                if (const auto at_end_p = ending_atoms.find(p); at_end_p != ending_atoms.cend())
                    for (const auto &a : at_end_p->second)
                        overlapping_atoms.erase(a);

                // if there are more than one atom in the set of overlapping atoms, we have an inconsistency..
                if (overlapping_atoms.size() > 1)
                {
                    has_conflict = true;
                    // we compute the possible resolvers..
                    std::vector<std::pair<utils::lit, double>> choices;
                    std::unordered_set<VARIABLE_TYPE> vars;
                    // we consider all the pairs of atoms in the Minimal Conflict Sets (MCSs)..
                    for (const auto &as : utils::combinations(std::vector<atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2))
                    {
                        std::set<atom *> mcs(as.cbegin(), as.cend()); // the MCS..
                        // we check if the MCS has already been found..
                        if (sv_flaws.find(mcs) == sv_flaws.cend())
                        {
                            // we add the MCS to the set of found flaws..
                            sv_flaws.insert(mcs);
                            get_solver().get_graph().new_flaw<sv_flaw>(*this, *sv, mcs);
                        }

                        const auto a0_start = std::static_pointer_cast<riddle::arith_item>(as[0]->get(START_NAME))->get_value();
                        const auto a0_end = std::static_pointer_cast<riddle::arith_item>(as[0]->get(END_NAME))->get_value();
                        const auto a1_start = std::static_pointer_cast<riddle::arith_item>(as[1]->get(START_NAME))->get_value();
                        const auto a1_end = std::static_pointer_cast<riddle::arith_item>(as[1]->get(END_NAME))->get_value();

                        if (auto a0_it = leqs.find(as[0]); a0_it != leqs.end())
                            if (auto a1_it = a0_it->second.find(as[1]); a1_it != a0_it->second.end())
                                if (get_solver().get_sat().value(a1_it->second) == utils::Undefined && vars.insert(variable(a1_it->second)).second)
                                    choices.push_back({a1_it->second, commit(a0_start, a0_end, a1_start, a1_end)});
                        if (auto a1_it = leqs.find(as[1]); a1_it != leqs.end())
                            if (auto a0_it = a1_it->second.find(as[0]); a0_it != a1_it->second.end())
                                if (get_solver().get_sat().value(a0_it->second) == utils::Undefined && vars.insert(variable(a0_it->second)).second)
                                    choices.push_back({a0_it->second, commit(a1_start, a1_end, a0_start, a0_end)});
                        for (const auto atm : as)
                            if (auto frb_it = frbs.find(atm); frb_it != frbs.end())
                            {
                                auto nr_frbs = std::count_if(frb_it->second.cbegin(), frb_it->second.cend(), [this](const auto &frb)
                                                             { return get_solver().get_sat().value(frb.second) == utils::Undefined; });
                                for (const auto &frb : frb_it->second)
                                    if (get_solver().get_sat().value(frb.second) == utils::Undefined && vars.insert(variable(frb.second)).second)
                                        choices.push_back({frb.second, 1. - 1. / nr_frbs});
                            }
                    }
                    incs.push_back(std::move(choices));
                }
            }

            if (!has_conflict) // the state-variable instance is consistent..
                to_check.erase(sv);
        }
        return incs;
    }

    void state_variable::new_atom(std::shared_ptr<ratio::atom> &atm) noexcept
    {
        if (atm->is_fact())
        {
            assert(is_interval(*atm));
            set_ni(atm->get_sigma());
            get_solver().get_predicate(INTERVAL_PREDICATE_NAME)->get().call(atm);
            restore_ni();
        }

        // we store the variables for on-line flaw resolution..
        // the ordering constraints between the atoms are stored in the leqs map..
        const auto start = atm->get(START_NAME);
        const auto end = atm->get(END_NAME);
        for (const auto &c_atm : atoms)
        {
            const auto c_start = c_atm.get().get(START_NAME);
            const auto c_end = c_atm.get().get(END_NAME);
#ifdef DL_TN
            auto before = get_solver().get_rdl_theory().new_leq(std::static_pointer_cast<riddle::arith_item>(end)->get_value(), std::static_pointer_cast<riddle::arith_item>(c_start)->get_value());
            auto after = get_solver().get_rdl_theory().new_leq(std::static_pointer_cast<riddle::arith_item>(c_end)->get_value(), std::static_pointer_cast<riddle::arith_item>(start)->get_value());
#elif defined(LRA_TN)
            auto before = get_solver().get_lra_theory().new_leq(std::static_pointer_cast<riddle::arith_item>(end)->get_value(), std::static_pointer_cast<riddle::arith_item>(c_start)->get_value());
            auto after = get_solver().get_lra_theory().new_leq(std::static_pointer_cast<riddle::arith_item>(c_end)->get_value(), std::static_pointer_cast<riddle::arith_item>(start)->get_value());
            [[maybe_unused]] bool nc = get_solver().get_sat().new_clause({!before, !after}); // the ordering constraints are always disjunctive..
            assert(nc);
#endif
            if (get_solver().get_sat().value(before) == utils::Undefined)
                leqs[atm.get()][&c_atm.get()] = before;
            if (get_solver().get_sat().value(after) == utils::Undefined)
                leqs[&c_atm.get()][atm.get()] = after;
        }

        const auto tau = atm->get(riddle::TAU_NAME);
        if (is_enum(*tau))
            for (const auto &sv : get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
            {
                auto var = get_solver().get_ov_theory().allows(static_cast<const riddle::enum_item &>(*tau).get_value(), sv.get());
                if (get_solver().get_sat().value(var) == utils::Undefined)
                    frbs[atm.get()][&sv.get()] = var;
            }

        atoms.push_back(*atm);
        auto listener = std::make_unique<sv_atom_listener>(*this, *atm);
        listener->something_changed();
        listeners.push_back(std::move(listener));
    }

    double state_variable::commit(const utils::lin &a0_start, const utils::lin &a0_end, const utils::lin &a1_start, const utils::lin &a1_end) noexcept
    {
#ifdef DL_TN
        const auto [min, max] = get_solver().get_rdl_theory().distance(a0_end, a1_start);
        return is_infinite(min) || is_infinite(max) ? 0.5 : to_double((std::min(max.get_rational(), utils::rational::zero) - std::min(min.get_rational(), utils::rational::zero)) / (max.get_rational() - min.get_rational()));
#elif defined(LRA_TN)
        const auto work = (get_solver().get_lra_theory().value(a1_end).get_rational() - get_solver().get_lra_theory().value(a1_start).get_rational()) * (get_solver().get_lra_theory().value(a0_end).get_rational() - get_solver().get_lra_theory().value(a1_start).get_rational());
        return work == utils::rational::zero ? -std::numeric_limits<double>::max() : 1l - 1l / (static_cast<double>(work.numerator()) / work.denominator());
#endif
    }

#ifdef ENABLE_API
    json::json state_variable::extract() const noexcept
    {
        json::json tls(json::json_type::array);
        // we partition atoms for each state-variable they might insist on..
        std::unordered_map<riddle::component *, std::vector<atom *>> sv_instances;
        for (const auto &sv : get_instances())
            sv_instances.emplace(static_cast<riddle::component *>(sv.get()), std::vector<atom *>());
        for (const auto &atm : atoms)
            if (get_solver().get_sat().value(atm.get().get_sigma()) == utils::True)
            { // the atom is active..
                const auto tau = atm.get().get(riddle::TAU_NAME);
                if (is_enum(*tau)) // the `tau` parameter is a variable..
                    for (const auto &c_sv : get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
                        sv_instances.at(static_cast<riddle::component *>(&c_sv.get())).push_back(&atm.get());
                else // the `tau` parameter is a constant..
                    sv_instances.at(static_cast<riddle::component *>(tau.get())).push_back(&atm.get());
            }

        for (const auto &[sv, atms] : sv_instances)
        {
            json::json tl{{"id", get_id(*sv)}, {"type", STATE_VARIABLE_TYPE_NAME}, {"name", get_solver().guess_name(*sv)}};

            // for each pulse, the atoms starting at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> ending_atoms;
            // all the pulses of the timeline..
            std::set<utils::inf_rational> pulses;

            for (const auto &atm : atms)
            {
                const auto start = get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(atm->get(START_NAME)));
                const auto end = get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(atm->get(END_NAME)));
                starting_atoms[start].insert(atm);
                ending_atoms[end].insert(atm);
                pulses.insert(start);
                pulses.insert(end);
            }
            pulses.insert(get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(get_core().get(ORIGIN_NAME))));
            pulses.insert(get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(get_core().get(HORIZON_NAME))));

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
#endif

    state_variable::sv_flaw::sv_flaw(state_variable &sv_tp, const riddle::component &sv, const std::set<atom *> &mcs) : flaw(sv_tp.get_solver(), smart_type::get_resolvers(mcs)), sv_tp(sv_tp), sv(sv), mcs(mcs) {}
    void state_variable::sv_flaw::compute_resolvers()
    {
        std::unordered_set<VARIABLE_TYPE> vars;
        for (const auto &as : utils::combinations(std::vector<atom *>(mcs.cbegin(), mcs.cend()), 2))
        {
            if (const auto a0_it = sv_tp.leqs.find(as[0]); a0_it != sv_tp.leqs.cend())
                if (const auto a0_a1_it = a0_it->second.find(as[1]); a0_a1_it != a0_it->second.cend())
                    if (get_solver().get_sat().value(a0_a1_it->second) != utils::False && vars.insert(variable(a0_a1_it->second)).second)
                        get_solver().get_graph().new_resolver<order_resolver>(*this, a0_a1_it->second, *as[0], *as[1]);

            if (const auto a1_it = sv_tp.leqs.find(as[1]); a1_it != sv_tp.leqs.cend())
                if (const auto a1_a0_it = a1_it->second.find(as[0]); a1_a0_it != a1_it->second.cend())
                    if (get_solver().get_sat().value(a1_a0_it->second) != utils::False && vars.insert(variable(a1_a0_it->second)).second)
                        get_solver().get_graph().new_resolver<order_resolver>(*this, a1_a0_it->second, *as[1], *as[0]);
        }
        for (const auto atm : mcs)
            if (const auto atm_frbs = sv_tp.frbs.find(atm); atm_frbs != sv_tp.frbs.cend())
                for (const auto &[atm_sv, sv_sel] : atm_frbs->second)
                    if (get_solver().get_sat().value(sv_sel) != utils::False && vars.insert(variable(sv_sel)).second)
                        get_solver().get_graph().new_resolver<forbid_resolver>(*this, sv_sel, *atm, static_cast<riddle::component &>(*atm_sv));
    }

#ifdef ENABLE_API
    json::json state_variable::sv_flaw::get_data() const noexcept
    {
        json::json data{{"type", "sv_flaw"}};

        json::json atms(json::json_type::array);
        for (const auto &atm : mcs)
            atms.push_back(get_id(*atm));
        data["atoms"] = atms;

        return data;
    }
#endif

    state_variable::sv_flaw::order_resolver::order_resolver(sv_flaw &flw, const utils::lit &r, const atom &before, const atom &after) : resolver(flw, r, utils::rational::zero), before(before), after(after) {}

#ifdef ENABLE_API
    json::json state_variable::sv_flaw::order_resolver::get_data() const noexcept { return {{"type", "order"}, {"before", get_id(before)}, {"after", get_id(after)}}; }
#endif

    state_variable::sv_flaw::forbid_resolver::forbid_resolver(sv_flaw &flw, const utils::lit &r, atom &atm, riddle::component &itm) : resolver(flw, r, utils::rational::zero), atm(atm), itm(itm) {}

#ifdef ENABLE_API
    json::json state_variable::sv_flaw::forbid_resolver::get_data() const noexcept { return {{"type", "forbid"}, {"forbid_atom", get_id(atm)}, {"forbid_item", get_id(itm)}}; }
#endif

    state_variable::sv_atom_listener::sv_atom_listener(state_variable &sv, atom &a) : atom_listener(a), sv(sv) {}
    void state_variable::sv_atom_listener::something_changed()
    {
        if (sv.get_solver().get_sat().value(atm.get_sigma()) == utils::True)
        { // the atom is active
            const auto tau = atm.get(riddle::TAU_NAME);
            if (is_enum(*tau))
                for (const auto &c_sv : sv.get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
                    sv.to_check.insert(static_cast<const riddle::item *>(&c_sv.get()));
            else
                sv.to_check.insert(tau.get());
        }
    }
} // namespace ratio