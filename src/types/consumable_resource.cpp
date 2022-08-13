#include "consumable_resource.h"
#include "solver.h"
#include "field.h"
#include "atom_flaw.h"
#include "combinations.h"
#include "parser.h"
#include <stdexcept>
#include <list>

namespace ratio::solver
{
    consumable_resource::consumable_resource(solver &slv) : smart_type(slv, CONSUMABLE_RESOURCE_NAME)
    {
        // we add the 'initial_amount' field..
        new_field(*this, std::make_unique<ratio::core::field>(slv.get_real_type(), CONSUMABLE_RESOURCE_INITIAL_AMOUNT));
        // we add the 'capacity' field..
        new_field(*this, std::make_unique<ratio::core::field>(slv.get_real_type(), CONSUMABLE_RESOURCE_CAPACITY));
        // we add a constructor..
        std::vector<ratio::core::field_ptr> ctr_args;
        ctr_args.emplace_back(std::make_unique<ratio::core::field>(get_core().get_real_type(), CONSUMABLE_RESOURCE_INITIAL_AMOUNT));
        ctr_args.emplace_back(std::make_unique<ratio::core::field>(get_core().get_real_type(), CONSUMABLE_RESOURCE_CAPACITY));

        ctr_ins.emplace_back(riddle::id_token(0, 0, 0, 0, CONSUMABLE_RESOURCE_INITIAL_AMOUNT));
        ctr_ins.emplace_back(riddle::id_token(0, 0, 0, 0, CONSUMABLE_RESOURCE_CAPACITY));

        std::vector<std::unique_ptr<const riddle::ast::expression>> i_ia;
        i_ia.emplace_back(std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::id_expression *>(new ratio::core::id_expression({riddle::id_token(0, 0, 0, 0, CONSUMABLE_RESOURCE_INITIAL_AMOUNT)}))));
        ctr_ivs.emplace_back(std::move(i_ia));

        std::vector<std::unique_ptr<const riddle::ast::expression>> i_c;
        i_c.emplace_back(std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::id_expression *>(new ratio::core::id_expression({riddle::id_token(0, 0, 0, 0, CONSUMABLE_RESOURCE_CAPACITY)}))));
        ctr_ivs.emplace_back(std::move(i_c));

        auto l_ia_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::id_expression *>(new ratio::core::id_expression(std::vector<riddle::id_token>({riddle::id_token(0, 0, 0, 0, CONSUMABLE_RESOURCE_INITIAL_AMOUNT)}))));
        auto r_ia_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::real_literal_expression *>(new ratio::core::real_literal_expression(riddle::real_token(0, 0, 0, 0, semitone::rational::ZERO))));
        auto ia_ge_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::geq_expression *>(new ratio::core::geq_expression(std::move(l_ia_xpr), std::move(r_ia_xpr))));
        auto ia_stmnt = std::unique_ptr<const riddle::ast::statement>(static_cast<const riddle::ast::expression_statement *>(new ratio::core::expression_statement(std::move(ia_ge_xpr))));
        ctr_stmnts.emplace_back(std::move(ia_stmnt));

        auto l_c_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::id_expression *>(new ratio::core::id_expression(std::vector<riddle::id_token>({riddle::id_token(0, 0, 0, 0, CONSUMABLE_RESOURCE_CAPACITY)}))));
        auto r_c_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::real_literal_expression *>(new ratio::core::real_literal_expression(riddle::real_token(0, 0, 0, 0, semitone::rational::ZERO))));
        auto c_ge_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::geq_expression *>(new ratio::core::geq_expression(std::move(l_c_xpr), std::move(r_c_xpr))));
        auto c_stmnt = std::unique_ptr<const riddle::ast::statement>(static_cast<const riddle::ast::expression_statement *>(new ratio::core::expression_statement(std::move(c_ge_xpr))));
        ctr_stmnts.emplace_back(std::move(c_stmnt));

        new_constructor(std::make_unique<cr_constructor>(*this, std::move(ctr_args), ctr_ins, ctr_ivs, ctr_stmnts));

        auto l_u_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::id_expression *>(new ratio::core::id_expression(std::vector<riddle::id_token>({riddle::id_token(0, 0, 0, 0, CONSUMABLE_RESOURCE_USE_AMOUNT_NAME)}))));
        auto r_u_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::real_literal_expression *>(new ratio::core::real_literal_expression(riddle::real_token(0, 0, 0, 0, semitone::rational::ZERO))));
        auto u_ge_xpr = std::unique_ptr<const riddle::ast::expression>(static_cast<const riddle::ast::geq_expression *>(new ratio::core::geq_expression(std::move(l_u_xpr), std::move(r_u_xpr))));
        auto u_stmnt = std::unique_ptr<const riddle::ast::statement>(static_cast<const riddle::ast::expression_statement *>(new ratio::core::expression_statement(std::move(u_ge_xpr))));
        pred_stmnts.emplace_back(std::move(u_stmnt));

        // we add the 'Produce' predicate, without notifying neither the resource nor its supertypes..
        std::vector<ratio::core::field_ptr> prod_pred_args;
        prod_pred_args.push_back(std::make_unique<ratio::core::field>(get_core().get_real_type(), CONSUMABLE_RESOURCE_USE_AMOUNT_NAME));
        auto pp = std::make_unique<produce_predicate>(*this, std::move(prod_pred_args), pred_stmnts);
        p_pred = pp.get();
        ratio::core::type::new_predicate(std::move(pp));

        // we add the 'Consume' predicate, without notifying neither the resource nor its supertypes..
        std::vector<ratio::core::field_ptr> cons_pred_args;
        cons_pred_args.push_back(std::make_unique<ratio::core::field>(get_core().get_real_type(), CONSUMABLE_RESOURCE_USE_AMOUNT_NAME));
        auto cp = std::make_unique<consume_predicate>(*this, std::move(cons_pred_args), pred_stmnts);
        c_pred = cp.get();
        ratio::core::type::new_predicate(std::move(cp));
    }

    std::vector<std::vector<std::pair<semitone::lit, double>>> consumable_resource::get_current_incs()
    {
        std::vector<std::vector<std::pair<semitone::lit, double>>> incs;
        return incs;
    }

    void consumable_resource::new_predicate(ratio::core::predicate &pred) noexcept
    {
        assert(get_solver().is_interval(pred));
        assert(!c_pred || c_pred->is_assignable_from(pred) || !p_pred || p_pred->is_assignable_from(pred));
        // each consumable-resource predicate has a tau parameter indicating on which resource the atoms insist on..
        new_field(pred, std::make_unique<ratio::core::field>(static_cast<type &>(pred.get_scope()), TAU_KW));
    }
    void consumable_resource::new_atom_flaw(atom_flaw &f)
    {
        auto &atm = f.get_atom();
        if (f.is_fact)
        { // we apply produce-predicate or the consume-predicate whenever the fact becomes active..
            set_ni(semitone::lit(get_sigma(get_solver(), atm)));
            auto atm_expr = f.get_atom_expr();
            if (p_pred->is_assignable_from(atm.get_type()))
                p_pred->apply_rule(atm_expr);
            else
                c_pred->apply_rule(atm_expr);
            restore_ni();
        }

        // we avoid unification..
        if (!get_solver().get_sat_core()->new_clause({!f.get_phi(), semitone::lit(get_sigma(get_solver(), atm))}))
            throw ratio::core::unsolvable_exception();

        // since flaws might require planning, we can't store the variables for on-line flaw resolution..

        // we store, for the atom, its atom listener..
        atoms.emplace_back(&atm);
        listeners.emplace_back(std::make_unique<cr_atom_listener>(*this, atm));

        // we filter out those atoms which are not strictly active..
        if (get_solver().get_sat_core()->value(get_sigma(get_solver(), atm)) == semitone::True)
        {
            const auto c_scope = atm.get(TAU_KW);
            if (const auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope))        // the 'tau' parameter is a variable..
                for (const auto &val : get_solver().get_ov_theory().value(enum_scope->get_var())) // we check for all its allowed values..
                    to_check.insert(static_cast<const ratio::core::item *>(val));
            else // the 'tau' parameter is a constant..
                to_check.insert(&*c_scope);
        }
    }

    consumable_resource::cr_constructor::cr_constructor(consumable_resource &cr, std::vector<ratio::core::field_ptr> args, const std::vector<riddle::id_token> &ins, const std::vector<std::vector<std::unique_ptr<const riddle::ast::expression>>> &ivs, const std::vector<std::unique_ptr<const riddle::ast::statement>> &stmnts) : ratio::core::constructor(cr, std::move(args), ins, ivs, stmnts) {}

    consumable_resource::produce_predicate::produce_predicate(consumable_resource &cr, std::vector<ratio::core::field_ptr> args, const std::vector<std::unique_ptr<const riddle::ast::statement>> &stmnts) : predicate(cr, CONSUMABLE_RESOURCE_PRODUCE_PREDICATE_NAME, std::move(args), stmnts) { new_supertype(cr.get_solver().get_interval()); }
    consumable_resource::consume_predicate::consume_predicate(consumable_resource &cr, std::vector<ratio::core::field_ptr> args, const std::vector<std::unique_ptr<const riddle::ast::statement>> &stmnts) : predicate(cr, CONSUMABLE_RESOURCE_CONSUME_PREDICATE_NAME, std::move(args), stmnts) { new_supertype(cr.get_solver().get_interval()); }

    consumable_resource::cr_atom_listener::cr_atom_listener(consumable_resource &cr, ratio::core::atom &atm) : atom_listener(atm), cr(cr) {}

    void consumable_resource::cr_atom_listener::something_changed()
    {
        // we filter out those atoms which are not strictly active..
        if (cr.get_solver().get_sat_core()->value(get_sigma(cr.get_solver(), atm)) == semitone::True)
        {
            const auto c_scope = atm.get(TAU_KW);
            if (const auto enum_scope = dynamic_cast<ratio::core::enum_item *>(&*c_scope))           // the 'tau' parameter is a variable..
                for (const auto &val : cr.get_solver().get_ov_theory().value(enum_scope->get_var())) // we check for all its allowed values..
                    cr.to_check.insert(static_cast<const ratio::core::item *>(val));
            else // the 'tau' parameter is a constant..
                cr.to_check.insert(&*c_scope);
        }
    }
} // namespace ratio::solver
