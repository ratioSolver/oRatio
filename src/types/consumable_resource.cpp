#include <cassert>
#include "consumable_resource.hpp"
#include "solver.hpp"
#ifdef ENABLE_API
#include "solver_api.hpp"
#endif

namespace ratio
{
    consumable_resource::consumable_resource(solver &slv) : smart_type(slv, CONSUMABLE_RESOURCE_TYPE_NAME)
    {
        // we add the `initial_amount` field to the consumable-resource type..
        add_field(std::make_unique<riddle::field>(slv.get_real_type(), CONSUMABLE_RESOURCE_INITIAL_AMOUNT_NAME));
        // we add the `capacity` field to the consumable-resource type..
        add_field(std::make_unique<riddle::field>(slv.get_real_type(), CONSUMABLE_RESOURCE_CAPACITY_NAME));

        // we create the constructor's arguments..
        std::vector<std::unique_ptr<riddle::field>> args;
        args.push_back(std::make_unique<riddle::field>(slv.get_real_type(), CONSUMABLE_RESOURCE_INITIAL_AMOUNT_NAME));
        args.push_back(std::make_unique<riddle::field>(slv.get_real_type(), CONSUMABLE_RESOURCE_CAPACITY_NAME));
        // we create the constructor's initializations..
        std::vector<riddle::init_element> inits;
        std::vector<std::unique_ptr<riddle::expression>> initial_amount_args;
        initial_amount_args.push_back(std::make_unique<riddle::real_expression>(riddle::real_token(utils::rational::zero, 0, 0, 0, 0)));
        inits.emplace_back(riddle::id_token(CONSUMABLE_RESOURCE_INITIAL_AMOUNT_NAME, 0, 0, 0, 0), std::move(initial_amount_args));
        std::vector<std::unique_ptr<riddle::expression>> capacity_args;
        capacity_args.push_back(std::make_unique<riddle::real_expression>(riddle::real_token(utils::rational::zero, 0, 0, 0, 0)));
        inits.emplace_back(riddle::id_token(CONSUMABLE_RESOURCE_CAPACITY_NAME, 0, 0, 0, 0), std::move(capacity_args));
        // we create the constructor's body..
        std::vector<std::unique_ptr<riddle::statement>> body;
        body.push_back(std::make_unique<riddle::expression_statement>(std::make_unique<riddle::geq_expression>(std::make_unique<riddle::id_expression>(std::vector<riddle::id_token>{riddle::id_token(CONSUMABLE_RESOURCE_INITIAL_AMOUNT_NAME, 0, 0, 0, 0)}), std::make_unique<riddle::real_expression>(riddle::real_token(utils::rational::zero, 0, 0, 0, 0)))));
        body.push_back(std::make_unique<riddle::expression_statement>(std::make_unique<riddle::geq_expression>(std::make_unique<riddle::id_expression>(std::vector<riddle::id_token>{riddle::id_token(CONSUMABLE_RESOURCE_CAPACITY_NAME, 0, 0, 0, 0)}), std::make_unique<riddle::real_expression>(riddle::real_token(utils::rational::zero, 0, 0, 0, 0)))));
        // we add the constructor to the consumable-resource type..
        add_constructor(std::make_unique<riddle::constructor>(*this, std::move(args), std::move(inits), std::move(body)));

        // we create the `Produce` predicate's arguments..
        std::vector<std::unique_ptr<riddle::field>> produce_args;
        produce_args.push_back(std::make_unique<riddle::field>(slv.get_real_type(), CONSUMABLE_RESOURCE_AMOUNT_NAME));
        // we create the `Produce` predicate's body..
        std::vector<std::unique_ptr<riddle::statement>> produce_body;
        produce_body.push_back(std::make_unique<riddle::expression_statement>(std::make_unique<riddle::geq_expression>(std::make_unique<riddle::id_expression>(std::vector<riddle::id_token>{riddle::id_token(CONSUMABLE_RESOURCE_AMOUNT_NAME, 0, 0, 0, 0)}), std::make_unique<riddle::real_expression>(riddle::real_token(utils::rational::zero, 0, 0, 0, 0)))));
        // we create the `Produce` predicate..
        auto produce_pred = std::make_unique<riddle::predicate>(*this, CONSUMABLE_RESOURCE_PRODUCTION_PREDICATE_NAME, std::move(produce_args), std::move(produce_body));
        add_parent(*produce_pred, get_solver().get_predicate(INTERVAL_PREDICATE_NAME)->get());
        // we add the `Produce` predicate to the consumable-resource type..
        add_predicate(std::move(produce_pred));

        // we create the `Consume` predicate's arguments..
        std::vector<std::unique_ptr<riddle::field>> consume_args;
        consume_args.push_back(std::make_unique<riddle::field>(slv.get_real_type(), CONSUMABLE_RESOURCE_AMOUNT_NAME));
        // we create the `Consume` predicate's body..
        std::vector<std::unique_ptr<riddle::statement>> consume_body;
        consume_body.push_back(std::make_unique<riddle::expression_statement>(std::make_unique<riddle::geq_expression>(std::make_unique<riddle::id_expression>(std::vector<riddle::id_token>{riddle::id_token(CONSUMABLE_RESOURCE_AMOUNT_NAME, 0, 0, 0, 0)}), std::make_unique<riddle::real_expression>(riddle::real_token(utils::rational::zero, 0, 0, 0, 0)))));
        // we create the `Consume` predicate..
        auto consume_pred = std::make_unique<riddle::predicate>(*this, CONSUMABLE_RESOURCE_CONSUMPTION_PREDICATE_NAME, std::move(consume_args), std::move(consume_body));
        add_parent(*consume_pred, get_solver().get_predicate(INTERVAL_PREDICATE_NAME)->get());
        // we add the `Consume` predicate to the consumable-resource type..
        add_predicate(std::move(consume_pred));
    }

    void consumable_resource::new_atom(std::shared_ptr<ratio::atom> &atm) noexcept
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
            for (const auto &cr : get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
            {
                auto var = get_solver().get_ov_theory().allows(static_cast<const riddle::enum_item &>(*tau).get_value(), cr.get());
                if (get_solver().get_sat().value(var) == utils::Undefined)
                    frbs[atm.get()][&cr.get()] = var;
            }

        atoms.push_back(*atm);
        auto listener = std::make_unique<cr_atom_listener>(*this, *atm);
        listener->something_changed();
        listeners.push_back(std::move(listener));
    }

#ifdef ENABLE_API
    json::json consumable_resource::extract() const noexcept
    {
        json::json tls(json::json_type::array);
        // we partition atoms for each consumable-resource they might insist on..
        std::unordered_map<riddle::component *, std::vector<atom *>> cr_instances;
        for (const auto &cr : get_instances())
            cr_instances.emplace(static_cast<riddle::component *>(cr.get()), std::vector<atom *>());
        for (const auto &atm : atoms)
            if (get_solver().get_sat().value(atm.get().get_sigma()) == utils::True)
            { // the atom is active..
                const auto tau = atm.get().get(riddle::TAU_NAME);
                if (is_enum(*tau)) // the `tau` parameter is a variable..
                    for (const auto &c_cr : get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
                        cr_instances.at(static_cast<riddle::component *>(&c_cr.get())).push_back(&atm.get());
                else // the `tau` parameter is a constant..
                    cr_instances.at(static_cast<riddle::component *>(tau.get())).push_back(&atm.get());
            }

        for (const auto &[cres, atms] : cr_instances)
        {
            const auto c_initial_amount = get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(cres->get(CONSUMABLE_RESOURCE_INITIAL_AMOUNT_NAME)));
            json::json tl{{"id", get_id(*cres)}, {"type", CONSUMABLE_RESOURCE_TYPE_NAME}, {"name", get_solver().guess_name(*cres)}, {CONSUMABLE_RESOURCE_CAPACITY_NAME, to_json(get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(cres->get(CONSUMABLE_RESOURCE_CAPACITY_NAME))))}, {CONSUMABLE_RESOURCE_INITIAL_AMOUNT_NAME, to_json(c_initial_amount)}};

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
            utils::inf_rational c_val = c_initial_amount;
            for (p = std::next(p); p != pulses.end(); ++p)
            {
                json::json j_val{{"from", to_json(*std::prev(p))}, {"to", to_json(*p)}};

                json::json j_atms(json::json_type::array);
                utils::inf_rational c_angular_coefficient; // the concurrent resource update..
                for (const auto &atm : overlapping_atoms)
                {
                    auto amount = get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(atm->get(CONSUMABLE_RESOURCE_AMOUNT_NAME)));
                    auto c_coeff = get_predicate(CONSUMABLE_RESOURCE_PRODUCTION_PREDICATE_NAME).value().get().is_assignable_from(atm->get_type()) ? amount : -amount;
                    c_coeff /= (get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(atm->get(END_NAME))) - get_solver().arithmetic_value(*std::static_pointer_cast<riddle::arith_item>(atm->get(START_NAME)))).get_rational();
                    c_angular_coefficient += c_coeff;
                    j_atms.push_back(get_id(*atm));
                }
                j_val["atoms"] = std::move(j_atms);
                j_val[START_NAME] = to_json(c_val);
                c_val += (c_angular_coefficient * (*p - *std::prev(p)).get_rational());
                j_val[END_NAME] = to_json(c_val);

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

    consumable_resource::cr_atom_listener::cr_atom_listener(consumable_resource &cr, atom &a) : atom_listener(a), cr(cr) {}
    void consumable_resource::cr_atom_listener::something_changed()
    {
        if (cr.get_solver().get_sat().value(atm.get_sigma()) == utils::True)
        { // the atom is active
            const auto tau = atm.get(riddle::TAU_NAME);
            if (is_enum(*tau))
                for (const auto &c_cr : cr.get_solver().domain(static_cast<const riddle::enum_item &>(*tau)))
                    cr.to_check.insert(static_cast<const riddle::item *>(&c_cr.get()));
            else
                cr.to_check.insert(tau.get());
        }
    }
} // namespace ratio