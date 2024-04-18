#include <cassert>
#include "reusable_resource.hpp"
#include "graph.hpp"
#include "combinations.hpp"

namespace ratio
{
    reusable_resource::reusable_resource(solver &slv) : smart_type(slv, "ReusableResource") {}

    std::vector<std::vector<std::pair<utils::lit, double>>> reusable_resource::get_current_incs() noexcept
    {
        std::vector<std::vector<std::pair<utils::lit, double>>> incs; // the inconsistencies..
        // we assign the atoms to the reusable-resources that need to be checked..
        std::unordered_map<riddle::component *, std::vector<atom *>> rr_instances;
        for (const auto &atm : atoms)
            if (get_solver().get_sat().value(atm.get().get_sigma()) == utils::True)
            { // the atom is active..
                const auto tau = atm.get().get("tau");
                if (is_enum(*tau))
                { // the `tau` parameter is a variable..
                    for (const auto &c_rr : get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
                        if (to_check.find(static_cast<const riddle::item *>(&c_rr.get())) != to_check.end())
                            rr_instances[static_cast<riddle::component *>(&c_rr.get())].push_back(&atm.get());
                }
                else if (to_check.find(tau.get()) != to_check.end()) // the `tau` parameter is a constant..
                    rr_instances[static_cast<riddle::component *>(tau.get())].push_back(&atm.get());
            }

        // we detect inconsistencies for each of the reusable-resource instances..
        for ([[maybe_unused]] const auto &[rr, atms] : rr_instances)
        {
            // for each pulse, the atoms starting at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<utils::inf_rational, std::set<atom *>> ending_atoms;
            // all the pulses of the timeline, sorted..
            std::set<utils::inf_rational> pulses;
            // the resource capacity..
            auto c_capacity = get_solver().arithmetic_value(static_cast<riddle::arith_item &>(*rr->get("capacity")));

            for (const auto &atm : atms)
            {
                const auto start = get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(atm->get("start")));
                const auto end = get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(atm->get("end")));
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

                utils::inf_rational c_usage; // the concurrent resource usage..
                for (const auto &a : overlapping_atoms)
                    c_usage += get_solver().arithmetic_value(static_cast<riddle::arith_item &>(*a->get("usage")));

                // if the resource usage exceeds the resource capacity, we have a conflict..
                if (c_usage > c_capacity)
                {
                    has_conflict = true;
                    // we extract the Minimal Conflict Sets (MCSs)..
                    // we sort the overlapping atoms, according to their resource usage, in descending order..
                    std::vector<atom *> inc_atoms(overlapping_atoms.cbegin(), overlapping_atoms.cend());
                    std::sort(inc_atoms.begin(), inc_atoms.end(), [this](const auto &atm0, const auto &atm1)
                              { return get_solver().arithmetic_value(static_cast<riddle::arith_item &>(*atm0->get("usage"))) > get_solver().arithmetic_value(static_cast<riddle::arith_item &>(*atm1->get("usage"))); });

                    utils::inf_rational mcs_usage;       // the concurrent mcs resource usage..
                    auto mcs_begin = inc_atoms.cbegin(); // the beginning of the current mcs..
                    auto mcs_end = inc_atoms.cbegin();   // the end of the current mcs..
                    while (mcs_end != inc_atoms.cend())
                    {
                        // we increase the size of the current mcs..
                        while (mcs_usage <= c_capacity && mcs_end != inc_atoms.cend())
                        {
                            mcs_usage += get_solver().arithmetic_value(static_cast<riddle::arith_item &>(*(*mcs_end)->get("usage")));
                            ++mcs_end;
                        }

                        if (mcs_usage > c_capacity)
                        {
                            std::set<atom *> mcs(mcs_begin, mcs_end); // the MCS..
                            // we check if the MCS has already been found..
                            if (rr_flaws.find(mcs) == rr_flaws.cend())
                            {
                                // we add the MCS to the set of found flaws..
                                rr_flaws.insert(mcs);
                                get_solver().get_graph().new_flaw<rr_flaw>(*this, mcs);
                            }
                        }

                        // we decrease the size of the current mcs..
                        mcs_usage -= get_solver().arithmetic_value(static_cast<riddle::arith_item &>(*(*mcs_begin)->get("usage")));
                        assert(mcs_usage <= c_capacity);
                        ++mcs_begin;
                    }
                }
            }
        }
        return incs;
    }

    void reusable_resource::new_atom(std::shared_ptr<ratio::atom> &atm) noexcept
    {
        if (atm->is_fact())
        {
            assert(is_interval(*atm));
            set_ni(atm->get_sigma());
            get_solver().get_predicate("Interval")->get().call(atm);
            restore_ni();
        }

        // we store the variables for on-line flaw resolution..
        // the ordering constraints between the atoms are stored in the leqs map..
        const auto start = atm->get("start");
        const auto end = atm->get("end");
        for (const auto &c_atm : atoms)
        {
            const auto c_start = c_atm.get().get("start");
            const auto c_end = c_atm.get().get("end");
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

        const auto tau = atm->get("tau");
        if (is_enum(*tau))
            for (const auto &rr : get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
            {
                auto var = get_solver().get_ov_theory().allows(static_cast<const riddle::enum_item &>(*tau).get_value(), rr.get());
                if (get_solver().get_sat().value(var) == utils::Undefined)
                    frbs[atm.get()][&rr.get()] = var;
            }

        atoms.push_back(*atm);
        auto listener = std::make_unique<rr_atom_listener>(*this, *atm);
        listener->something_changed();
        listeners.push_back(std::move(listener));
    }

    reusable_resource::rr_flaw::rr_flaw(reusable_resource &rr, const std::set<atom *> &mcs) : flaw(rr.get_solver(), smart_type::get_resolvers(mcs)), rr(rr), mcs(mcs) {}
    void reusable_resource::rr_flaw::compute_resolvers() {}

    reusable_resource::rr_flaw::order_resolver::order_resolver(rr_flaw &flw, const utils::lit &r, const atom &before, const atom &after) : resolver(flw, r, utils::rational::zero), before(before), after(after) {}

    reusable_resource::rr_flaw::forbid_resolver::forbid_resolver(rr_flaw &flw, const utils::lit &r, atom &atm, riddle::component &itm) : resolver(flw, r, utils::rational::zero), atm(atm), itm(itm) {}

    reusable_resource::rr_atom_listener::rr_atom_listener(reusable_resource &rr, atom &a) : atom_listener(a), rr(rr) {}
    void reusable_resource::rr_atom_listener::something_changed()
    {
        if (rr.get_solver().get_sat().value(atm.get_sigma()) == utils::True)
        { // the atom is active
            const auto tau = atm.get("tau");
            if (is_enum(*tau))
                for (const auto &c_rr : rr.get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
                    rr.to_check.insert(static_cast<const riddle::item *>(&c_rr.get()));
            else
                rr.to_check.insert(tau.get());
        }
    }
} // namespace ratio