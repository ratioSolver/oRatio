#include <unordered_set>
#include <cassert>
#include "state_variable.hpp"
#include "graph.hpp"
#include "combinations.hpp"
#ifdef ENABLE_API
#include "solver_api.hpp"
#endif

namespace ratio
{
    state_variable::state_variable(solver &slv) : smart_type(slv, STATE_VARIABLE_TYPE_NAME) {}

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
                    // state-variable Minimal Conflict Sets (MCSs) are made of two atoms..
                    for (const auto &as : utils::combinations(std::vector<atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2))
                    {
                        std::set<atom *> mcs(as.cbegin(), as.cend()); // the MCS..
                        // we check if the MCS has already been found..
                        if (sv_flaws.find(mcs) == sv_flaws.cend())
                        {
                            // we add the MCS to the set of found flaws..
                            sv_flaws.insert(mcs);
                            get_solver().get_graph().new_flaw<sv_flaw>(*this, mcs);
                        }
                    }
                }
            }
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

    state_variable::sv_flaw::sv_flaw(state_variable &sv, const std::set<atom *> &mcs) : flaw(sv.get_solver(), smart_type::get_resolvers(mcs)), sv(sv), mcs(mcs) {}
    void state_variable::sv_flaw::compute_resolvers() {}

#ifdef ENABLE_API
    json::json state_variable::sv_flaw::get_data() const noexcept {}
#endif

    state_variable::sv_flaw::order_resolver::order_resolver(sv_flaw &flw, const utils::lit &r, const atom &before, const atom &after) : resolver(flw, r, utils::rational::zero), before(before), after(after) {}

#ifdef ENABLE_API
    json::json state_variable::sv_flaw::order_resolver::get_data() const noexcept {}
#endif

    state_variable::sv_flaw::forbid_resolver::forbid_resolver(sv_flaw &flw, const utils::lit &r, atom &atm, riddle::component &itm) : resolver(flw, r, utils::rational::zero), atm(atm), itm(itm) {}

#ifdef ENABLE_API
    json::json state_variable::sv_flaw::forbid_resolver::get_data() const noexcept {}
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