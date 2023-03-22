#include "consumable_resource.h"
#include "solver.h"
#include "atom_flaw.h"
#include "combinations.h"
#include <cassert>

namespace ratio
{
    consumable_resource::consumable_resource(riddle::scope &scp) : smart_type(scp, CONSUMABLE_RESOURCE_NAME)
    {
        // we add the `initial_amount` field..
        add_field(new riddle::field(get_solver().get_real_type(), CONSUMABLE_RESOURCE_INITIAL_AMOUNT));
        // we add the `capacity` parameter..
        add_field(new riddle::field(get_solver().get_real_type(), CONSUMABLE_RESOURCE_CAPACITY));

        // we create the constructor's arguments..
        std::vector<riddle::field_ptr> ctr_args;
        ctr_args.push_back(new riddle::field(get_solver().get_real_type(), CONSUMABLE_RESOURCE_INITIAL_AMOUNT));
        ctr_args.push_back(new riddle::field(get_solver().get_real_type(), CONSUMABLE_RESOURCE_CAPACITY));

        // we create the constructor's initialization list..
        ctr_ins.emplace_back(0, 0, 0, 0, CONSUMABLE_RESOURCE_INITIAL_AMOUNT);
        ctr_ins.emplace_back(0, 0, 0, 0, CONSUMABLE_RESOURCE_CAPACITY);
        std::vector<riddle::ast::expression_ptr> ia_ivs;
        ia_ivs.push_back(new riddle::ast::id_expression({riddle::id_token(0, 0, 0, 0, CONSUMABLE_RESOURCE_INITIAL_AMOUNT)}));
        ctr_ivs.emplace_back(std::move(ia_ivs));
        std::vector<riddle::ast::expression_ptr> cap_ivs;
        cap_ivs.push_back(new riddle::ast::id_expression({riddle::id_token(0, 0, 0, 0, CONSUMABLE_RESOURCE_CAPACITY)}));
        ctr_ivs.emplace_back(std::move(cap_ivs));

        // we create the constructor's body..
        ctr_body.emplace_back(new riddle::ast::expression_statement(new riddle::ast::geq_expression(new riddle::ast::id_expression({riddle::id_token(0, 0, 0, 0, CONSUMABLE_RESOURCE_CAPACITY)}), new riddle::ast::real_literal_expression({riddle::real_token(0, 0, 0, 0, utils::rational::ZERO)}))));

        // we add the constructor..
        add_constructor(new riddle::constructor(*this, std::move(ctr_args), ctr_ins, ctr_ivs, ctr_body));

        // we create the `Produce` predicate's arguments..
        std::vector<riddle::field_ptr> prod_args;
        prod_args.push_back(new riddle::field(get_solver().get_real_type(), CONSUMABLE_RESOURCE_AMOUNT_NAME));

        // we create the `Consume` predicate's arguments..
        std::vector<riddle::field_ptr> cons_args;
        cons_args.push_back(new riddle::field(get_solver().get_real_type(), CONSUMABLE_RESOURCE_AMOUNT_NAME));

        // we create the predicates' body..
        pred_body.push_back(new riddle::ast::expression_statement(new riddle::ast::geq_expression(new riddle::ast::id_expression({riddle::id_token(0, 0, 0, 0, CONSUMABLE_RESOURCE_AMOUNT_NAME)}), new riddle::ast::real_literal_expression({riddle::real_token(0, 0, 0, 0, utils::rational::ZERO)}))));

        // we add the `Produce` predicate..
        p_pred = new riddle::predicate(*this, CONSUMABLE_RESOURCE_PRODUCE_PREDICATE_NAME, std::move(prod_args), pred_body);
        add_parent(*p_pred, get_solver().get_interval());
        add_predicate(p_pred);

        // we add the `Consume` predicate..
        c_pred = new riddle::predicate(*this, CONSUMABLE_RESOURCE_CONSUME_PREDICATE_NAME, std::move(cons_args), pred_body);
        add_parent(*c_pred, get_solver().get_interval());
        add_predicate(c_pred);
    }

    std::vector<std::vector<std::pair<semitone::lit, double>>> consumable_resource::get_current_incs()
    {
        std::vector<std::vector<std::pair<semitone::lit, double>>> incs;
        // we partition atoms for each consumable-resource they might insist on..
        std::unordered_map<const riddle::complex_item *, std::vector<atom *>> cr_instances;
        for (const auto &atm : atoms)
            if (get_solver().get_sat_core().value(atm->get_sigma()) == utils::True) // we filter out those atoms which are not strictly active..
            {
                const auto cr = atm->get(TAU_KW); // we get the consumable-resource..
                if (auto enum_scope = dynamic_cast<enum_item *>(cr.operator->()))
                { // the `tau` parameter is a variable..
                    for (const auto &cr_val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        if (to_check.count(static_cast<const riddle::complex_item *>(cr_val))) // we consider only those consumable-resources which are still to be checked..
                            cr_instances[static_cast<const riddle::complex_item *>(cr_val)].emplace_back(atm);
                }
                else if (to_check.count(static_cast<riddle::complex_item *>(cr.operator->()))) // we consider only those consumable-resources which are still to be checked..
                    cr_instances[static_cast<riddle::complex_item *>(cr.operator->())].emplace_back(atm);
            }

        return incs;
    }

    void consumable_resource::new_atom(atom &atm)
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
        if (const auto tau_enum = dynamic_cast<enum_item *>(tau.operator->()))
        { // the `tau` parameter is a variable..
            const auto vals = get_solver().get_ov_theory().value(tau_enum->get_var());
            if (vals.size() > 1)
                for (const auto &val : vals)
                {
                    auto var = get_solver().get_ov_theory().allows(tau_enum->get_var(), *val);
                    if (get_solver().get_sat_core().value(var) == utils::Undefined)
                        frbs[&atm][dynamic_cast<riddle::item *>(val)] = !var;
                }
        }

        atoms.emplace_back(&atm);
        // we store, for the atom, its atom listener..
        listeners.emplace_back(new cr_atom_listener(*this, atm));

        // we filter out those atoms which are not strictly active..
        if (get_solver().get_sat_core().value(atm.get_sigma()) == utils::True)
        {
            const auto c_scope = atm.get(TAU_KW);
            if (const auto enum_scope = dynamic_cast<enum_item *>(c_scope.operator->()))          // the `tau` parameter is a variable..
                for (const auto &val : get_solver().get_ov_theory().value(enum_scope->get_var())) // we check for all its allowed values..
                    to_check.insert(dynamic_cast<const riddle::item *>(val));
            else // the `tau` parameter is a constant..
                to_check.insert(&*c_scope);
        }
    }

    void consumable_resource::cr_atom_listener::something_changed()
    {
        if (cr.get_solver().get_sat_core().value(atm.get_sigma()) == utils::True)
        { // the atom is active..
            const auto a0_tau = atm.get(TAU_KW);
            cr.to_check.insert(&atm);
            if (const auto a0_tau_itm = dynamic_cast<enum_item *>(a0_tau.operator->())) // the `tau` parameter is a variable..
                for (const auto &val : cr.get_solver().get_ov_theory().value(a0_tau_itm->get_var()))
                    cr.to_check.insert(dynamic_cast<const riddle::item *>(val));
            else // the `tau` parameter is a constant..
                cr.to_check.insert(a0_tau.operator->());
        }
    }

    consumable_resource::cr_flaw::cr_flaw(consumable_resource &cr, const std::set<atom *> &atms) : flaw(cr.get_solver(), smart_type::get_resolvers(atms)), cr(cr), overlapping_atoms(atms) {}

    json::json consumable_resource::cr_flaw::get_data() const noexcept
    {
        json::json data;
        data["type"] = "cr-flaw";

        json::json atms(json::json_type::array);
        for (const auto &atm : overlapping_atoms)
            atms.push_back(get_id(*atm));
        data["atoms"] = atms;

        return data;
    }

    void consumable_resource::cr_flaw::compute_resolvers() { throw std::runtime_error("not implemented"); }

    consumable_resource::cr_flaw::order_resolver::order_resolver(cr_flaw &flw, const semitone::lit &r, const atom &before, const atom &after) : resolver(flw, r, utils::rational::ZERO), before(before), after(after) {}

    json::json consumable_resource::cr_flaw::order_resolver::get_data() const noexcept
    {
        json::json data;
        data["type"] = "order";
        data["before"] = get_id(before);
        data["after"] = get_id(after);
        return data;
    }

    consumable_resource::cr_flaw::forbid_resolver::forbid_resolver(cr_flaw &flw, atom &atm, riddle::item &itm) : resolver(flw, semitone::lit(), utils::rational::ZERO), atm(atm), itm(itm) {}

    json::json consumable_resource::cr_flaw::forbid_resolver::get_data() const noexcept
    {
        json::json data;
        data["type"] = "forbid";
        data["forbid_atom"] = get_id(atm);
        data["forbid_item"] = get_id(itm);
        return data;
    }

    json::json consumable_resource::extract() const noexcept
    {
        json::json tls(json::json_type::array);
        // we partition atoms for each consumable-resource they might insist on..
        std::unordered_map<riddle::complex_item *, std::vector<atom *>> cr_instances;
        for (const auto &atm : atoms)
            if (get_solver().get_sat_core().value(atm->get_sigma()) == utils::True) // we filter out those atoms which are not strictly active..
            {
                const auto cr = atm->get(TAU_KW); // we get the consumable-resource..
                if (auto enum_scope = dynamic_cast<enum_item *>(cr.operator->()))
                { // the `tau` parameter is a variable..
                    for (const auto &cr_val : get_solver().get_ov_theory().value(enum_scope->get_var()))
                        if (to_check.count(static_cast<riddle::complex_item *>(cr_val))) // we consider only those consumable-resources which are still to be checked..
                            cr_instances[static_cast<riddle::complex_item *>(cr_val)].emplace_back(atm);
                }
                else if (to_check.count(static_cast<riddle::complex_item *>(cr.operator->()))) // we consider only those consumable-resources which are still to be checked..
                    cr_instances[static_cast<riddle::complex_item *>(cr.operator->())].emplace_back(atm);
            }

        for (const auto &[cres, atms] : cr_instances)
        {
            const auto c_initial_amount = get_solver().arith_value(cres->get(CONSUMABLE_RESOURCE_INITIAL_AMOUNT));
            json::json tl{{"id", get_id(*cres)}, {"type", CONSUMABLE_RESOURCE_NAME}, {"capacity", to_json(get_solver().arith_value(cres->get(CONSUMABLE_RESOURCE_CAPACITY)))}, {"initial_amount", to_json(c_initial_amount)}};
#ifdef COMPUTE_NAMES
            tl["name"] = get_solver().guess_name(*cres);
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
            utils::inf_rational c_val = c_initial_amount;
            for (p = std::next(p); p != pulses.end(); ++p)
            {
                json::json j_val{{"from", to_json(*std::prev(p))}, {"to", to_json(*p)}};

                json::json j_atms(json::json_type::array);
                utils::inf_rational c_angular_coefficient; // the concurrent resource update..
                for (const auto &atm : overlapping_atoms)
                {
                    auto c_coeff = get_produce_predicate().is_assignable_from(atm->get_type()) ? get_solver().arith_value(atm->get(CONSUMABLE_RESOURCE_AMOUNT_NAME)) : -get_solver().arith_value(atm->get(CONSUMABLE_RESOURCE_AMOUNT_NAME));
                    c_coeff /= (get_solver().arith_value(atm->get(RATIO_END)) - get_solver().arith_value(atm->get(RATIO_START))).get_rational();
                    c_angular_coefficient += c_coeff;
                    j_atms.push_back(get_id(*atm));
                }
                j_val["atoms"] = std::move(j_atms);
                j_val["start"] = to_json(c_val);
                c_val += (c_angular_coefficient * (*p - *std::prev(p)).get_rational());
                j_val["end"] = to_json(c_val);

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
