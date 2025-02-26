#include "types.hpp"
#include "item.hpp"
#include <sstream>
#include <set>

namespace ratio
{
    state_variable::state_variable(graph &gr) noexcept : component_type(gr, state_variable_kw) { add_constructor(utils::make_u_ptr<riddle::constructor>(*this)); }

    void state_variable::created_predicate(riddle::predicate &pred) { add_parent(pred, get_core().get_predicate(interval_kw)); }

    json::json state_variable::extract() const
    {
        json::json tls(json::json_type::array);
        // we partition atoms for each state-variable they might insist on..
        std::unordered_map<const riddle::component *, std::vector<riddle::atom_term *>> sv_instances;
        for (const auto &sv : get_instances())
            sv_instances.emplace(static_cast<riddle::component *>(&*sv), std::vector<riddle::atom_term *>());
        for (const auto &atm : get_atoms())
            if (atm->get_state() == riddle::atom_state::active)
            { // the atom is active..
                const auto tau = atm->get(riddle::tau_kw);
                if (auto c_svs = dynamic_cast<riddle::enum_item *>(&*tau)) // the `tau` parameter is a variable..
                    for (const auto &c_sv : get_core().enum_value(*c_svs))
                        sv_instances.at(static_cast<riddle::component *>(&*c_sv)).push_back(&*atm);
                else // the `tau` parameter is a constant..
                    sv_instances.at(static_cast<riddle::component *>(tau.get())).push_back(&*atm);
            }

        for (const auto &[sv, atms] : sv_instances)
        {
            json::json tl{{"id", static_cast<uint64_t>(sv->get_id())}, {"type", state_variable_kw}};
#ifdef COMPUTE_NAMES
            tl["name"] = guess_name(*sv).c_str();
#endif

            // for each pulse, the atoms starting at that pulse..
            std::map<utils::inf_rational, std::set<riddle::atom_term *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<utils::inf_rational, std::set<riddle::atom_term *>> ending_atoms;
            // all the pulses of the timeline..
            std::set<utils::inf_rational> pulses;

            for (const auto &atm : atms)
            {
                const auto start = get_core().arith_value(static_cast<riddle::arith_term &>(*atm->get(riddle::start_kw)));
                const auto end = get_core().arith_value(static_cast<riddle::arith_term &>(*atm->get(riddle::end_kw)));
                starting_atoms[start].insert(atm);
                ending_atoms[end].insert(atm);
                pulses.insert(start);
                pulses.insert(end);
            }
            pulses.insert(get_core().arith_value(static_cast<riddle::arith_term &>(*get_core().get(origin_kw))));
            pulses.insert(get_core().arith_value(static_cast<riddle::arith_term &>(*get_core().get(horizon_kw))));

            std::set<riddle::atom_term *> overlapping_atoms;
            std::set<utils::inf_rational>::iterator p = pulses.begin();
            if (const auto at_start_p = starting_atoms.find(*p); at_start_p != starting_atoms.cend())
                overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
            if (const auto at_end_p = ending_atoms.find(*p); at_end_p != ending_atoms.cend())
                for (const auto &a : at_end_p->second)
                    overlapping_atoms.erase(a);

            json::json j_vals(json::json_type::array);
            for (p = std::next(p); p != pulses.end(); ++p)
            {
                json::json j_val{{"from", {{"num", static_cast<int64_t>(std::prev(p)->get_rational().numerator())}, {"den", static_cast<int64_t>(std::prev(p)->get_rational().denominator())}}}, {"to", {{"num", static_cast<int64_t>(p->get_rational().numerator())}, {"den", static_cast<int64_t>(p->get_rational().denominator())}}}};

                json::json j_atms(json::json_type::array);
                for (const auto &atm : overlapping_atoms)
                    j_atms.push_back(static_cast<uint64_t>(atm->get_id()));
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

    reusable_resource::reusable_resource(graph &gr) noexcept : component_type(gr, reusable_resource_kw)
    {
        add_field(utils::make_u_ptr<riddle::field>(gr.get_type(riddle::real_kw), reusable_resource_capacity_kw, nullptr));

        std::istringstream ctr_decl("ReusableResource(real capacity) : capacity(capacity) { capacity >= 0.0; }");
        riddle::parser ctr_p(ctr_decl);
        ctr = ctr_p.parse_constructor_declaration();
        ctr->refine(*this);

        std::istringstream use_pred_decl("predicate Use(real amount) : Interval { amount >= 0.0; }");
        riddle::parser use_pred_p(use_pred_decl);
        use_pred = use_pred_p.parse_predicate_declaration();
        use_pred->declare(*this);
        use_pred->refine(*this);
    }

    json::json reusable_resource::extract() const
    {
        json::json tls(json::json_type::array);
        // we partition atoms for each state-variable they might insist on..
        std::unordered_map<const riddle::component *, std::vector<riddle::atom_term *>> rr_instances;
        for (const auto &rr : get_instances())
            rr_instances.emplace(static_cast<riddle::component *>(&*rr), std::vector<riddle::atom_term *>());
        for (const auto &atm : get_atoms())
            if (atm->get_state() == riddle::atom_state::active)
            { // the atom is active..
                const auto tau = atm->get(riddle::tau_kw);
                if (auto c_rrs = dynamic_cast<riddle::enum_item *>(&*tau)) // the `tau` parameter is a variable..
                    for (const auto &c_rr : get_core().enum_value(*c_rrs))
                        rr_instances.at(static_cast<riddle::component *>(&*c_rr)).push_back(&*atm);
                else // the `tau` parameter is a constant..
                    rr_instances.at(static_cast<riddle::component *>(tau.get())).push_back(&*atm);
            }

        for (const auto &[rr, atms] : rr_instances)
        {
            json::json tl{{"id", static_cast<uint64_t>(rr->get_id())}, {"type", reusable_resource_kw}};
#ifdef COMPUTE_NAMES
            tl["name"] = guess_name(*rr).c_str();
#endif

            // for each pulse, the atoms starting at that pulse..
            std::map<utils::inf_rational, std::set<riddle::atom_term *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<utils::inf_rational, std::set<riddle::atom_term *>> ending_atoms;
            // all the pulses of the timeline..
            std::set<utils::inf_rational> pulses;

            for (const auto &atm : atms)
            {
                const auto start = get_core().arith_value(static_cast<riddle::arith_term &>(*atm->get(riddle::start_kw)));
                const auto end = get_core().arith_value(static_cast<riddle::arith_term &>(*atm->get(riddle::end_kw)));
                starting_atoms[start].insert(atm);
                ending_atoms[end].insert(atm);
                pulses.insert(start);
                pulses.insert(end);
            }
            pulses.insert(get_core().arith_value(static_cast<riddle::arith_term &>(*get_core().get(origin_kw))));
            pulses.insert(get_core().arith_value(static_cast<riddle::arith_term &>(*get_core().get(horizon_kw))));

            std::set<riddle::atom_term *> overlapping_atoms;
            std::set<utils::inf_rational>::iterator p = pulses.begin();
            if (const auto at_start_p = starting_atoms.find(*p); at_start_p != starting_atoms.cend())
                overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
            if (const auto at_end_p = ending_atoms.find(*p); at_end_p != ending_atoms.cend())
                for (const auto &a : at_end_p->second)
                    overlapping_atoms.erase(a);

            json::json j_vals(json::json_type::array);
            for (p = std::next(p); p != pulses.end(); ++p)
            {
                json::json j_val{{"from", {{"num", static_cast<int64_t>(std::prev(p)->get_rational().numerator())}, {"den", static_cast<int64_t>(std::prev(p)->get_rational().denominator())}}}, {"to", {{"num", static_cast<int64_t>(p->get_rational().numerator())}, {"den", static_cast<int64_t>(p->get_rational().denominator())}}}};

                json::json j_atms(json::json_type::array);
                for (const auto &atm : overlapping_atoms)
                    j_atms.push_back(static_cast<uint64_t>(atm->get_id()));
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

    consumable_resource::consumable_resource(graph &gr) noexcept : component_type(gr, consumable_resource_kw)
    {
        add_field(utils::make_u_ptr<riddle::field>(gr.get_type(riddle::real_kw), consumable_resource_capacity_kw, nullptr));
        add_field(utils::make_u_ptr<riddle::field>(gr.get_type(riddle::real_kw), consumable_resource_initial_amount_kw, nullptr));

        std::istringstream ctr_decl("ConsumableResource(real capacity, real initial_amount) : capacity(capacity), initial_amount(initial_amount) { capacity >= 0.0; initial_amount <= capacity; }");
        riddle::parser ctr_p(ctr_decl);
        ctr = ctr_p.parse_constructor_declaration();
        ctr->refine(*this);

        std::istringstream prod_pred_decl("predicate Produce(real amount) : Interval { amount >= 0.0; }");
        riddle::parser prod_pred_p(prod_pred_decl);
        prod_pred = prod_pred_p.parse_predicate_declaration();
        prod_pred->declare(*this);
        prod_pred->refine(*this);

        std::istringstream cons_pred_decl("predicate Consume(real amount) : Interval { amount >= 0.0; }");
        riddle::parser cons_pred_p(cons_pred_decl);
        cons_pred = cons_pred_p.parse_predicate_declaration();
        cons_pred->declare(*this);
        cons_pred->refine(*this);
    }

    json::json consumable_resource::extract() const
    {
        json::json tls(json::json_type::array);
        // we partition atoms for each state-variable they might insist on..
        std::unordered_map<const riddle::component *, std::vector<riddle::atom_term *>> cr_instances;
        for (const auto &cr : get_instances())
            cr_instances.emplace(static_cast<riddle::component *>(&*cr), std::vector<riddle::atom_term *>());
        for (const auto &atm : get_atoms())
            if (atm->get_state() == riddle::atom_state::active)
            { // the atom is active..
                const auto tau = atm->get(riddle::tau_kw);
                if (auto c_crs = dynamic_cast<riddle::enum_item *>(&*tau)) // the `tau` parameter is a variable..
                    for (const auto &c_cr : get_core().enum_value(*c_crs))
                        cr_instances.at(static_cast<riddle::component *>(&*c_cr)).push_back(&*atm);
                else // the `tau` parameter is a constant..
                    cr_instances.at(static_cast<riddle::component *>(tau.get())).push_back(&*atm);
            }

        for (const auto &[cr, atms] : cr_instances)
        {
            json::json tl{{"id", static_cast<uint64_t>(cr->get_id())}, {"type", consumable_resource_kw}};
#ifdef COMPUTE_NAMES
            tl["name"] = guess_name(*cr).c_str();
#endif

            // for each pulse, the atoms starting at that pulse..
            std::map<utils::inf_rational, std::set<riddle::atom_term *>> starting_atoms;
            // for each pulse, the atoms ending at that pulse..
            std::map<utils::inf_rational, std::set<riddle::atom_term *>> ending_atoms;
            // all the pulses of the timeline..
            std::set<utils::inf_rational> pulses;

            for (const auto &atm : atms)
            {
                const auto start = get_core().arith_value(static_cast<riddle::arith_term &>(*atm->get(riddle::start_kw)));
                const auto end = get_core().arith_value(static_cast<riddle::arith_term &>(*atm->get(riddle::end_kw)));
                starting_atoms[start].insert(atm);
                ending_atoms[end].insert(atm);
                pulses.insert(start);
                pulses.insert(end);
            }
            pulses.insert(get_core().arith_value(static_cast<riddle::arith_term &>(*get_core().get(origin_kw))));
            pulses.insert(get_core().arith_value(static_cast<riddle::arith_term &>(*get_core().get(horizon_kw))));

            std::set<riddle::atom_term *> overlapping_atoms;
            std::set<utils::inf_rational>::iterator p = pulses.begin();
            if (const auto at_start_p = starting_atoms.find(*p); at_start_p != starting_atoms.cend())
                overlapping_atoms.insert(at_start_p->second.cbegin(), at_start_p->second.cend());
            if (const auto at_end_p = ending_atoms.find(*p); at_end_p != ending_atoms.cend())
                for (const auto &a : at_end_p->second)
                    overlapping_atoms.erase(a);

            json::json j_vals(json::json_type::array);
            for (p = std::next(p); p != pulses.end(); ++p)
            {
                json::json j_val{{"from", {{"num", static_cast<int64_t>(std::prev(p)->get_rational().numerator())}, {"den", static_cast<int64_t>(std::prev(p)->get_rational().denominator())}}}, {"to", {{"num", static_cast<int64_t>(p->get_rational().numerator())}, {"den", static_cast<int64_t>(p->get_rational().denominator())}}}};

                json::json j_atms(json::json_type::array);
                for (const auto &atm : overlapping_atoms)
                    j_atms.push_back(static_cast<uint64_t>(atm->get_id()));
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
