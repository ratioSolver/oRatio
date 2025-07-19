#include "sttypes.hpp"
#include "stsolver.hpp"
#include "combinations.hpp"
#include "stflaws.hpp"
#include "logging.hpp"
#include <sstream>
#include <cassert>

namespace ratio
{
    atom_listener::atom_listener(stcomponent_type &ct, atom &atm) noexcept : prop_listener(static_cast<solver &>(atm.get_core())), la_listener(static_cast<solver &>(atm.get_core()).get_linear_arithmetic_theory()), ct(ct), atm(atm)
    {
        listen(variable(atm.get_sigma()));
        for (const auto &[name, xpr] : atm.items)
            if (auto *ov = dynamic_cast<const riddle::enum_item *>(xpr.get()))
            {
                for (const auto &v : atm.get_core().enum_value(*ov))
                    listen(variable(ov->get_lit(v.get())));
            }
            else if (is_bool(xpr))
            {
                if (static_cast<solver &>(atm.get_core()).value(static_cast<riddle::bool_item &>(*xpr).get_lit()) == utils::Undefined)
                    listen(variable(static_cast<riddle::bool_item &>(*xpr).get_lit()));
            }
            else if (is_arith(xpr))
            {
                const auto lb = static_cast<solver &>(atm.get_core()).arith_lb(static_cast<riddle::arith_item &>(*xpr).get_lin());
                const auto ub = static_cast<solver &>(atm.get_core()).arith_ub(static_cast<riddle::arith_item &>(*xpr).get_lin());
                if (lb < ub)
                    for (const auto &l : static_cast<const riddle::arith_item &>(*xpr).get_lin().vars)
                        listen_arith(l.first);
            }

        auto tau = atm.get(riddle::tau_kw);
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(tau.get()))
            for (const auto &v : atm.get_core().enum_value(*ov))
                ct.to_check.emplace(dynamic_cast<riddle::component *>(&v.get()));
        else
            ct.to_check.emplace(dynamic_cast<riddle::component *>(tau.get()));
    }
    void atom_listener::on_change(const utils::var &) noexcept
    {
        auto tau = atm.get(riddle::tau_kw);
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(tau.get()))
            for (const auto &v : atm.get_core().enum_value(*ov))
                ct.to_check.emplace(dynamic_cast<riddle::component *>(&v.get()));
        else
            ct.to_check.emplace(dynamic_cast<riddle::component *>(tau.get()));
    }
    void atom_listener::on_arith_change(const utils::var &) noexcept
    {
        auto tau = atm.get(riddle::tau_kw);
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(tau.get()))
            for (const auto &v : atm.get_core().enum_value(*ov))
                ct.to_check.emplace(dynamic_cast<riddle::component *>(&v.get()));
        else
            ct.to_check.emplace(dynamic_cast<riddle::component *>(tau.get()));
    }

    stcomponent_type::stcomponent_type(solver &slv) noexcept : slv(slv) {}
    utils::lbool stcomponent_type::share_component(riddle::atom_expr lhs, riddle::atom_expr rhs)
    {
        auto l_tau = lhs->get(riddle::tau_kw);
        auto r_tau = rhs->get(riddle::tau_kw);
        if (l_tau == r_tau)
            return utils::True; // the atoms must be on the same component..

        std::set<const riddle::component *> l_tau_cmps;
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(l_tau.get()))
            for (const auto &v : get_solver().enum_value(*ov))
                l_tau_cmps.emplace(dynamic_cast<riddle::component *>(&v.get()));
        else
            l_tau_cmps.emplace(dynamic_cast<riddle::component *>(l_tau.get()));

        std::set<const riddle::component *> r_tau_cmps;
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(r_tau.get()))
            for (const auto &v : get_solver().enum_value(*ov))
                r_tau_cmps.emplace(dynamic_cast<riddle::component *>(&v.get()));
        else
            r_tau_cmps.emplace(dynamic_cast<riddle::component *>(r_tau.get()));

        if (l_tau_cmps.size() == 1 && r_tau_cmps.size() == 1 && *l_tau_cmps.begin() == *r_tau_cmps.begin())
            return utils::True; // the atoms must be on the same component..
        for (const auto &r : r_tau_cmps)
            if (l_tau_cmps.count(r))
                return utils::Undefined; // the atoms might be on the same component..

        return utils::False; // the atoms can't be on the same component..
    }

    void stcomponent_type::new_flaw(std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<utils::lit> &&clause)
    {
        if (slv.decision_level() == 0)
            slv.new_flaw<clause_flaw>(slv, std::move(causes), std::move(clause), false);
        else
            pending_flaws.push_back({std::move(causes), std::move(clause)});
    }

    ststate_variable::ststate_variable(solver &slv) noexcept : state_variable(slv), stcomponent_type(slv) {}

    std::vector<std::vector<std::pair<utils::lit, double>>> ststate_variable::get_current_incs() noexcept
    {
        std::vector<std::vector<std::pair<utils::lit, double>>> incs; // the inconsistencies..
        // we assign the atoms to the state-variables that need to be checked..
        std::unordered_map<const riddle::component *, std::vector<riddle::atom_term *>> sv_instances;
        for (const auto &atm : get_atoms())
            if (get_solver().value(std::dynamic_pointer_cast<atom>(atm)->get_sigma()) == utils::True)
            { // the atom is active..
                const auto tau = atm->get(riddle::tau_kw);
                if (auto c_svs = dynamic_cast<riddle::enum_item *>(tau.get())) // the `tau` parameter is a variable..
                    for (const auto &c_sv : get_core().enum_value(*c_svs))
                        sv_instances[dynamic_cast<riddle::component *>(&c_sv.get())].push_back(atm.get());
                else // the `tau` parameter is a constant..
                    sv_instances[dynamic_cast<riddle::component *>(tau.get())].push_back(atm.get());
            }

        for (const auto &[sv, atms] : sv_instances)
            if (to_check.count(sv))
            {
                // for each pulse, the atoms starting at that pulse..
                std::map<utils::inf_rational, std::set<riddle::atom_term *>> starting_atoms;
                // for each pulse, the atoms ending at that pulse..
                std::map<utils::inf_rational, std::set<riddle::atom_term *>> ending_atoms;
                // all the pulses of the timeline..
                std::set<utils::inf_rational> pulses;

                for (const auto &atm : atms)
                {
                    const auto start = get_core().arith_value(*atm->get<riddle::arith_term>(riddle::start_kw));
                    const auto end = get_core().arith_value(*atm->get<riddle::arith_term>(riddle::end_kw));
                    starting_atoms[start].insert(atm);
                    ending_atoms[end].insert(atm);
                    pulses.insert(start);
                    pulses.insert(end);
                }
                pulses.insert(get_core().arith_value(*get_core().env::get<riddle::arith_term>(origin_kw)));
                pulses.insert(get_core().arith_value(*get_core().env::get<riddle::arith_term>(horizon_kw)));

                // we scroll through the timeline looking for inconsistencies..
                bool has_conflict = false;
                std::set<riddle::atom_term *> overlapping_atoms;
                std::set<utils::inf_rational>::iterator p = pulses.begin();
                if (const auto at_start_p = starting_atoms.find(*p); at_start_p != starting_atoms.cend())
                    overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
                if (const auto at_end_p = ending_atoms.find(*p); at_end_p != ending_atoms.cend())
                    for (const auto &a : at_end_p->second)
                        overlapping_atoms.erase(a);

                for (p = std::next(p); p != pulses.end(); ++p)
                {
                    if (overlapping_atoms.size() > 1) // if there are more than one atom in the set of overlapping atoms, we have an inconsistency..
                    {
                        has_conflict = true;
                        // we compute the possible resolvers..
                        std::vector<std::pair<utils::lit, double>> choices;
                        std::unordered_set<utils::var> vars;
                        // we consider all the pairs of atoms in the Minimal Conflict Sets (MCSs)..
                        for (const auto &as : utils::combinations(std::vector<riddle::atom_term *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2))
                        {
                            const auto a0_start = static_cast<riddle::arith_item &>(*as[0]->get(riddle::start_kw)).get_lin();
                            const auto a0_end = static_cast<riddle::arith_item &>(*as[0]->get(riddle::end_kw)).get_lin();
                            const auto a1_start = static_cast<riddle::arith_item &>(*as[1]->get(riddle::start_kw)).get_lin();
                            const auto a1_end = static_cast<riddle::arith_item &>(*as[1]->get(riddle::end_kw)).get_lin();

                            std::vector<utils::lit> cs;
                            if (auto a0_it = leqs.find(as[0]); a0_it != leqs.end())
                                if (auto a1_it = a0_it->second.find(as[1]); a1_it != a0_it->second.end())
                                    if (get_solver().value(a1_it->second) == utils::Undefined && vars.insert(variable(a1_it->second)).second)
                                    {
                                        cs.push_back(a1_it->second);
                                        auto work = (get_solver().arith_val(a1_end).get_rational() - get_solver().arith_val(a1_start).get_rational()) * (get_solver().arith_val(a0_end).get_rational() - get_solver().arith_val(a1_start).get_rational());
                                        choices.push_back({a1_it->second, 1l - 1l / (static_cast<double>(work.numerator()) / work.denominator())});
                                    }
                            if (auto a1_it = leqs.find(as[1]); a1_it != leqs.end())
                                if (auto a0_it = a1_it->second.find(as[0]); a0_it != a1_it->second.end())
                                    if (get_solver().value(a0_it->second) == utils::Undefined && vars.insert(variable(a0_it->second)).second)
                                    {
                                        cs.push_back(a0_it->second);
                                        auto work = (get_solver().arith_val(a0_end).get_rational() - get_solver().arith_val(a0_start).get_rational()) * (get_solver().arith_val(a1_end).get_rational() - get_solver().arith_val(a0_start).get_rational());
                                        choices.push_back({a0_it->second, 1l - 1l / (static_cast<double>(work.numerator()) / work.denominator())});
                                    }
                            for (const auto atm : as)
                                if (auto frb_it = frbs.find(atm); frb_it != frbs.end())
                                {
                                    auto nr_frbs = std::count_if(frb_it->second.cbegin(), frb_it->second.cend(), [this](const auto &frb)
                                                                 { return get_solver().value(frb.second) == utils::Undefined; });
                                    for (const auto &frb : frb_it->second)
                                        if (get_solver().value(frb.second) == utils::Undefined && vars.insert(variable(frb.second)).second)
                                        {
                                            cs.push_back(frb.second);
                                            choices.push_back({frb.second, 1. - 1. / nr_frbs});
                                        }
                                }

                            std::set<riddle::atom_term *> mcs(as.cbegin(), as.cend()); // the MCS..
                            if (sv_flaws.insert(mcs).second && get_solver().decision_level())
                            {
                                std::vector<std::reference_wrapper<resolver>> causes;
                                for (const auto &a : as)
                                    for (const auto &r : static_cast<atom *>(a)->get_flaw().get_resolvers())
                                        if (auto act = dynamic_cast<activate_fact *>(&r.get()))
                                        {
                                            causes.emplace_back(*act);
                                            break;
                                        }
                                        else if (auto act = dynamic_cast<activate_goal *>(&r.get()))
                                        {
                                            causes.emplace_back(*act);
                                            break;
                                        }
                                new_flaw(std::move(causes), std::move(cs));
                            }
                        }
                        incs.push_back(std::move(choices));
                    }

                    if (const auto at_start_p = starting_atoms.find(*p); at_start_p != starting_atoms.cend())
                        overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
                    if (const auto at_end_p = ending_atoms.find(*p); at_end_p != ending_atoms.cend())
                        for (const auto &a : at_end_p->second)
                            overlapping_atoms.erase(a);
                }

                if (!has_conflict) // the state-variable instance is consistent..
                    to_check.erase(sv);
            }
        return incs;
    }

    void ststate_variable::created_atom(riddle::atom_expr atm) noexcept
    {
        if (atm->is_fact())
            get_core().get_predicate(interval_kw).call(atm);

        // we store the variables for on-line flaw resolution..
        auto tau = atm->get(riddle::tau_kw);
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(tau.get()))
            for (const auto &v : get_solver().enum_value(*ov))
                frbs[atm.get()][&v.get()] = ov->get_lit(v.get());

        const auto start = std::dynamic_pointer_cast<riddle::arith_item>(atm->get(riddle::start_kw));
        const auto end = std::dynamic_pointer_cast<riddle::arith_item>(atm->get(riddle::end_kw));
        for (const auto &c_atm : get_atoms())
            if (atm != c_atm)
            {
                switch (share_component(atm, c_atm))
                {
                case utils::True:
                { // the atoms are on the same state-variable..
                    const auto c_start = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::start_kw));
                    const auto c_end = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::end_kw));

                    if (get_solver().arith_ub(end->get_lin()) > get_solver().arith_lb(c_start->get_lin()) && get_solver().arith_lb(start->get_lin()) < get_solver().arith_ub(c_end->get_lin()))
                    { // the atoms might temporally overlap..
                        if (get_solver().arith_ub(start->get_lin()) < get_solver().arith_lb(c_end->get_lin()))
                            get_solver().add_le(end->get_lin(), c_start->get_lin()); // `atm` must be before `c_atm`..
                        else if (get_solver().arith_lb(end->get_lin()) > get_solver().arith_ub(c_start->get_lin()))
                            get_solver().add_le(c_end->get_lin(), start->get_lin()); // `c_atm` must be before `atm`..
                        else
                        { // the ordering constraints between the atoms are stored in the leqs map..
                            auto before = utils::lit(get_solver().mk_var());
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after);  // `c_atm` before `atm`..
                            assert(get_solver().value(before) == utils::Undefined && get_solver().value(after) == utils::Undefined);
                            leqs[atm.get()][c_atm.get()] = before;
                            leqs[c_atm.get()][atm.get()] = after;
                        }
                    }
                }
                break;
                case utils::Undefined:
                { // the atoms might be on the same state-variable..
                    const auto c_start = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::start_kw));
                    const auto c_end = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::end_kw));

                    if (get_solver().arith_ub(end->get_lin()) > get_solver().arith_lb(c_start->get_lin()) && get_solver().arith_lb(start->get_lin()) < get_solver().arith_ub(c_end->get_lin()))
                    { // the atoms might temporally overlap..
                        if (get_solver().arith_ub(start->get_lin()) < get_solver().arith_lb(c_end->get_lin()))
                        {
                            auto before = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            assert(get_solver().value(before) == utils::Undefined);
                            leqs[atm.get()][c_atm.get()] = before;
                        }
                        else if (get_solver().arith_lb(end->get_lin()) > get_solver().arith_ub(c_start->get_lin()))
                        {
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after); // `c_atm` before `atm`..
                            assert(get_solver().value(after) == utils::Undefined);
                            leqs[c_atm.get()][atm.get()] = after;
                        }
                        else
                        { // the ordering constraints between the atoms are stored in the leqs map..
                            auto before = utils::lit(get_solver().mk_var());
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after);  // `c_atm` before `atm`..
                            assert(get_solver().value(before) == utils::Undefined && get_solver().value(after) == utils::Undefined);
                            leqs[atm.get()][c_atm.get()] = before;
                            leqs[c_atm.get()][atm.get()] = after;
                        }
                    }
                }
                break;
                }
            }

        listeners.emplace_back(*this, static_cast<ratio::atom &>(*atm));
    }

    streusable_resource::streusable_resource(solver &slv) noexcept : reusable_resource(slv), stcomponent_type(slv) {}

    std::vector<std::vector<std::pair<utils::lit, double>>> streusable_resource::get_current_incs() noexcept
    {
        std::vector<std::vector<std::pair<utils::lit, double>>> incs; // the inconsistencies..
        // we assign the atoms to the state-variables that need to be checked..
        std::unordered_map<riddle::component *, std::vector<riddle::atom_term *>> rr_instances;
        for (const auto &atm : get_atoms())
            if (get_solver().value(std::dynamic_pointer_cast<atom>(atm)->get_sigma()) == utils::True)
            { // the atom is active..
                const auto tau = atm->get(riddle::tau_kw);
                if (auto c_svs = dynamic_cast<riddle::enum_item *>(tau.get())) // the `tau` parameter is a variable..
                    for (const auto &c_sv : get_core().enum_value(*c_svs))
                        rr_instances[dynamic_cast<riddle::component *>(&c_sv.get())].push_back(atm.get());
                else // the `tau` parameter is a constant..
                    rr_instances[dynamic_cast<riddle::component *>(tau.get())].push_back(atm.get());
            }

        for (const auto &[rr, atms] : rr_instances)
            if (to_check.count(rr))
            {
                // for each pulse, the atoms starting at that pulse..
                std::map<utils::inf_rational, std::set<riddle::atom_term *>> starting_atoms;
                // for each pulse, the atoms ending at that pulse..
                std::map<utils::inf_rational, std::set<riddle::atom_term *>> ending_atoms;
                // all the pulses of the timeline..
                std::set<utils::inf_rational> pulses;
                // the resource capacity..
                const auto c_capacity = get_core().arith_value(*rr->get<riddle::arith_term>(riddle::reusable_resource_capacity_kw));

                for (const auto &atm : atms)
                {
                    const auto start = get_core().arith_value(*atm->get<riddle::arith_term>(riddle::start_kw));
                    const auto end = get_core().arith_value(*atm->get<riddle::arith_term>(riddle::end_kw));
                    starting_atoms[start].insert(atm);
                    ending_atoms[end].insert(atm);
                    pulses.insert(start);
                    pulses.insert(end);
                }
                pulses.insert(get_core().arith_value(*get_core().env::get<riddle::arith_term>(origin_kw)));
                pulses.insert(get_core().arith_value(*get_core().env::get<riddle::arith_term>(horizon_kw)));

                // we scroll through the timeline looking for inconsistencies..
                bool has_conflict = false;
                std::set<riddle::atom_term *> overlapping_atoms;
                std::set<utils::inf_rational>::iterator p = pulses.begin();
                if (const auto at_start_p = starting_atoms.find(*p); at_start_p != starting_atoms.cend())
                    overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
                if (const auto at_end_p = ending_atoms.find(*p); at_end_p != ending_atoms.cend())
                    for (const auto &a : at_end_p->second)
                        overlapping_atoms.erase(a);

                for (p = std::next(p); p != pulses.end(); ++p)
                {
                    utils::inf_rational c_usage; // the concurrent resource usage..
                    for (const auto &a : overlapping_atoms)
                        c_usage += get_core().arith_value(*a->get<riddle::arith_term>(riddle::reusable_resource_amount_kw));

                    if (c_usage > c_capacity) // if the resource usage exceeds the resource capacity, we have a conflict..
                    {
                        has_conflict = true;
                        // we compute the possible resolvers..
                        std::vector<std::pair<utils::lit, double>> choices;
                        std::unordered_set<utils::var> vars;
                        // we consider all the pairs of atoms in the Minimal Conflict Sets (MCSs)..
                        for (const auto &as : utils::combinations(std::vector<riddle::atom_term *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2))
                        {
                            const auto a0_start = static_cast<riddle::arith_item &>(*as[0]->get(riddle::start_kw)).get_lin();
                            const auto a0_end = static_cast<riddle::arith_item &>(*as[0]->get(riddle::end_kw)).get_lin();
                            const auto a1_start = static_cast<riddle::arith_item &>(*as[1]->get(riddle::start_kw)).get_lin();
                            const auto a1_end = static_cast<riddle::arith_item &>(*as[1]->get(riddle::end_kw)).get_lin();

                            std::vector<utils::lit> cs;
                            if (auto a0_it = leqs.find(as[0]); a0_it != leqs.end())
                                if (auto a1_it = a0_it->second.find(as[1]); a1_it != a0_it->second.end())
                                    if (get_solver().value(a1_it->second) == utils::Undefined && vars.insert(variable(a1_it->second)).second)
                                    {
                                        cs.push_back(a1_it->second);
                                        auto work = (get_solver().arith_val(a1_end).get_rational() - get_solver().arith_val(a1_start).get_rational()) * (get_solver().arith_val(a0_end).get_rational() - get_solver().arith_val(a1_start).get_rational());
                                        choices.push_back({a1_it->second, 1l - 1l / (static_cast<double>(work.numerator()) / work.denominator())});
                                    }
                            if (auto a1_it = leqs.find(as[1]); a1_it != leqs.end())
                                if (auto a0_it = a1_it->second.find(as[0]); a0_it != a1_it->second.end())
                                    if (get_solver().value(a0_it->second) == utils::Undefined && vars.insert(variable(a0_it->second)).second)
                                    {
                                        cs.push_back(a0_it->second);
                                        auto work = (get_solver().arith_val(a0_end).get_rational() - get_solver().arith_val(a0_start).get_rational()) * (get_solver().arith_val(a1_end).get_rational() - get_solver().arith_val(a0_start).get_rational());
                                        choices.push_back({a0_it->second, 1l - 1l / (static_cast<double>(work.numerator()) / work.denominator())});
                                    }
                            for (const auto atm : as)
                                if (auto frb_it = frbs.find(atm); frb_it != frbs.end())
                                {
                                    auto nr_frbs = std::count_if(frb_it->second.cbegin(), frb_it->second.cend(), [this](const auto &frb)
                                                                 { return get_solver().value(frb.second) == utils::Undefined; });
                                    for (const auto &frb : frb_it->second)
                                        if (get_solver().value(frb.second) == utils::Undefined && vars.insert(variable(frb.second)).second)
                                        {
                                            cs.push_back(frb.second);
                                            choices.push_back({frb.second, 1. - 1. / nr_frbs});
                                        }
                                }

                            std::set<riddle::atom_term *> mcs(as.cbegin(), as.cend()); // the MCS..
                            if (rr_flaws.insert(mcs).second && get_solver().decision_level())
                            {
                                std::vector<std::reference_wrapper<resolver>> causes;
                                for (const auto &a : as)
                                    for (const auto &r : static_cast<atom *>(a)->get_flaw().get_resolvers())
                                        if (auto act = dynamic_cast<activate_fact *>(&r.get()))
                                        {
                                            causes.emplace_back(*act);
                                            break;
                                        }
                                        else if (auto act = dynamic_cast<activate_goal *>(&r.get()))
                                        {
                                            causes.emplace_back(*act);
                                            break;
                                        }
                                new_flaw(std::move(causes), std::move(cs));
                            }
                        }
                        incs.push_back(std::move(choices));
                    }

                    if (const auto at_start_p = starting_atoms.find(*p); at_start_p != starting_atoms.cend())
                        overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
                    if (const auto at_end_p = ending_atoms.find(*p); at_end_p != ending_atoms.cend())
                        for (const auto &a : at_end_p->second)
                            overlapping_atoms.erase(a);
                }

                if (!has_conflict) // the state-variable instance is consistent..
                    to_check.erase(rr);
            }
        return incs;
    }

    void streusable_resource::created_atom(riddle::atom_expr atm) noexcept
    {
        if (atm->is_fact())
            get_core().get_predicate(interval_kw).call(atm);

        // we store the variables for on-line flaw resolution..
        auto tau = atm->get(riddle::tau_kw);
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(tau.get()))
            for (const auto &v : get_solver().enum_value(*ov))
                frbs[atm.get()][&v.get()] = ov->get_lit(v.get());

        const auto start = std::dynamic_pointer_cast<riddle::arith_item>(atm->get(riddle::start_kw));
        const auto end = std::dynamic_pointer_cast<riddle::arith_item>(atm->get(riddle::end_kw));
        for (const auto &c_atm : get_atoms())
            if (atm != c_atm)
            {
                switch (share_component(atm, c_atm))
                {
                case utils::True:
                { // the atoms are on the same state-variable..
                    const auto c_start = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::start_kw));
                    const auto c_end = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::end_kw));

                    if (get_solver().arith_ub(end->get_lin()) > get_solver().arith_lb(c_start->get_lin()) && get_solver().arith_lb(start->get_lin()) < get_solver().arith_ub(c_end->get_lin()))
                    { // the atoms might temporally overlap..
                        if (get_solver().arith_ub(start->get_lin()) < get_solver().arith_lb(c_end->get_lin()))
                            get_solver().add_le(end->get_lin(), c_start->get_lin()); // `atm` must be before `c_atm`..
                        else if (get_solver().arith_lb(end->get_lin()) > get_solver().arith_ub(c_start->get_lin()))
                            get_solver().add_le(c_end->get_lin(), start->get_lin()); // `c_atm` must be before `atm`..
                        else
                        { // the ordering constraints between the atoms are stored in the leqs map..
                            auto before = utils::lit(get_solver().mk_var());
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after);  // `c_atm` before `atm`..
                            assert(get_solver().value(before) == utils::Undefined && get_solver().value(after) == utils::Undefined);
                            leqs[atm.get()][c_atm.get()] = before;
                            leqs[c_atm.get()][atm.get()] = after;
                        }
                    }
                }
                break;
                case utils::Undefined:
                { // the atoms might be on the same state-variable..
                    const auto c_start = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::start_kw));
                    const auto c_end = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::end_kw));

                    if (get_solver().arith_ub(end->get_lin()) > get_solver().arith_lb(c_start->get_lin()) && get_solver().arith_lb(start->get_lin()) < get_solver().arith_ub(c_end->get_lin()))
                    { // the atoms might temporally overlap..
                        if (get_solver().arith_ub(start->get_lin()) < get_solver().arith_lb(c_end->get_lin()))
                        {
                            auto before = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            assert(get_solver().value(before) == utils::Undefined);
                            leqs[atm.get()][c_atm.get()] = before;
                        }
                        else if (get_solver().arith_lb(end->get_lin()) > get_solver().arith_ub(c_start->get_lin()))
                        {
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after); // `c_atm` before `atm`..
                            assert(get_solver().value(after) == utils::Undefined);
                            leqs[c_atm.get()][atm.get()] = after;
                        }
                        else
                        { // the ordering constraints between the atoms are stored in the leqs map..
                            auto before = utils::lit(get_solver().mk_var());
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after);  // `c_atm` before `atm`..
                            assert(get_solver().value(before) == utils::Undefined && get_solver().value(after) == utils::Undefined);
                            leqs[atm.get()][c_atm.get()] = before;
                            leqs[c_atm.get()][atm.get()] = after;
                        }
                    }
                }
                break;
                }
            }

        listeners.emplace_back(*this, static_cast<ratio::atom &>(*atm));
    }

    stconsumable_resource::stconsumable_resource(solver &slv) noexcept : consumable_resource(slv), stcomponent_type(slv) {}

    std::vector<std::vector<std::pair<utils::lit, double>>> stconsumable_resource::get_current_incs() noexcept
    {
        std::vector<std::vector<std::pair<utils::lit, double>>> incs; // the inconsistencies..
        return incs;
    }

    void stconsumable_resource::created_atom(riddle::atom_expr atm) noexcept
    {
        if (atm->is_fact())
            get_core().get_predicate(interval_kw).call(atm);

        // we store the variables for on-line flaw resolution..
        auto tau = atm->get(riddle::tau_kw);
        if (auto *ov = dynamic_cast<const riddle::enum_item *>(tau.get()))
            for (const auto &v : get_solver().enum_value(*ov))
                frbs[atm.get()][&v.get()] = ov->get_lit(v.get());

        const auto start = std::dynamic_pointer_cast<riddle::arith_item>(atm->get(riddle::start_kw));
        const auto end = std::dynamic_pointer_cast<riddle::arith_item>(atm->get(riddle::end_kw));
        for (const auto &c_atm : get_atoms())
            if (atm != c_atm)
            {
                switch (share_component(atm, c_atm))
                {
                case utils::True:
                { // the atoms are on the same state-variable..
                    const auto c_start = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::start_kw));
                    const auto c_end = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::end_kw));

                    if (get_solver().arith_ub(end->get_lin()) > get_solver().arith_lb(c_start->get_lin()) && get_solver().arith_lb(start->get_lin()) < get_solver().arith_ub(c_end->get_lin()))
                    { // the atoms might temporally overlap..
                        if (get_solver().arith_ub(start->get_lin()) < get_solver().arith_lb(c_end->get_lin()))
                            get_solver().add_le(end->get_lin(), c_start->get_lin()); // `atm` must be before `c_atm`..
                        else if (get_solver().arith_lb(end->get_lin()) > get_solver().arith_ub(c_start->get_lin()))
                            get_solver().add_le(c_end->get_lin(), start->get_lin()); // `c_atm` must be before `atm`..
                        else
                        { // the ordering constraints between the atoms are stored in the leqs map..
                            auto before = utils::lit(get_solver().mk_var());
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after);  // `c_atm` before `atm`..
                            assert(get_solver().value(before) == utils::Undefined && get_solver().value(after) == utils::Undefined);
                            leqs[atm.get()][c_atm.get()] = before;
                            leqs[c_atm.get()][atm.get()] = after;
                        }
                    }
                }
                break;
                case utils::Undefined:
                { // the atoms might be on the same state-variable..
                    const auto c_start = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::start_kw));
                    const auto c_end = std::dynamic_pointer_cast<riddle::arith_item>(c_atm->get(riddle::end_kw));

                    if (get_solver().arith_ub(end->get_lin()) > get_solver().arith_lb(c_start->get_lin()) && get_solver().arith_lb(start->get_lin()) < get_solver().arith_ub(c_end->get_lin()))
                    { // the atoms might temporally overlap..
                        if (get_solver().arith_ub(start->get_lin()) < get_solver().arith_lb(c_end->get_lin()))
                        {
                            auto before = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            assert(get_solver().value(before) == utils::Undefined);
                            leqs[atm.get()][c_atm.get()] = before;
                        }
                        else if (get_solver().arith_lb(end->get_lin()) > get_solver().arith_ub(c_start->get_lin()))
                        {
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after); // `c_atm` before `atm`..
                            assert(get_solver().value(after) == utils::Undefined);
                            leqs[c_atm.get()][atm.get()] = after;
                        }
                        else
                        { // the ordering constraints between the atoms are stored in the leqs map..
                            auto before = utils::lit(get_solver().mk_var());
                            auto after = utils::lit(get_solver().mk_var());
                            get_solver().add_le(end->get_lin(), c_start->get_lin(), before); // `atm` before `c_atm`..
                            get_solver().add_le(c_end->get_lin(), start->get_lin(), after);  // `c_atm` before `atm`..
                            assert(get_solver().value(before) == utils::Undefined && get_solver().value(after) == utils::Undefined);
                            leqs[atm.get()][c_atm.get()] = before;
                            leqs[c_atm.get()][atm.get()] = after;
                        }
                    }
                }
                break;
                }
            }

        listeners.emplace_back(*this, static_cast<ratio::atom &>(*atm));
    }
} // namespace ratio
