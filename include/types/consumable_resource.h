#pragma once

#include "smart_type.h"
#include "constructor.h"
#include "predicate.h"
#include "flaw.h"
#include "resolver.h"

#define CONSUMABLE_RESOURCE_NAME "ConsumableResource"
#define CONSUMABLE_RESOURCE_INITIAL_AMOUNT "initial_amount"
#define CONSUMABLE_RESOURCE_CAPACITY "capacity"
#define CONSUMABLE_RESOURCE_PRODUCE_PREDICATE_NAME "Produce"
#define CONSUMABLE_RESOURCE_CONSUME_PREDICATE_NAME "Consume"
#define CONSUMABLE_RESOURCE_USE_AMOUNT_NAME "amount"

namespace ratio::solver
{
  class consumable_resource final : public smart_type
  {
  public:
    consumable_resource(solver &slv);
    consumable_resource(const consumable_resource &orig) = delete;

  private:
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() override;

    void new_predicate(ratio::core::predicate &pred) noexcept override;
    void new_atom_flaw(atom_flaw &f) override;

    // the consumable-resource constructor..
    class cr_constructor final : public ratio::core::constructor
    {
    public:
      cr_constructor(consumable_resource &rr, std::vector<ratio::core::field_ptr> args, const std::vector<riddle::id_token> &ins, const std::vector<std::vector<std::unique_ptr<const riddle::ast::expression>>> &ivs, const std::vector<std::unique_ptr<const riddle::ast::statement>> &stmnts);
      cr_constructor(cr_constructor &&) = delete;
    };

    // the consumable-resource 'Produce' predicate..
    class produce_predicate final : public ratio::core::predicate
    {
    public:
      produce_predicate(consumable_resource &rr, std::vector<ratio::core::field_ptr> args, const std::vector<std::unique_ptr<const riddle::ast::statement>> &stmnts);
      produce_predicate(produce_predicate &&) = delete;
    };

    // the consumable-resource 'Consume' predicate..
    class consume_predicate final : public ratio::core::predicate
    {
    public:
      consume_predicate(consumable_resource &rr, std::vector<ratio::core::field_ptr> args, const std::vector<std::unique_ptr<const riddle::ast::statement>> &stmnts);
      consume_predicate(consume_predicate &&) = delete;
    };

    // the atom listener for the consumable-resource..
    class cr_atom_listener final : public atom_listener
    {
    public:
      cr_atom_listener(consumable_resource &rr, ratio::core::atom &atm);
      cr_atom_listener(cr_atom_listener &&) = delete;

    private:
      void something_changed();

      void sat_value_change(const semitone::var &) override { something_changed(); }
      void rdl_value_change(const semitone::var &) override { something_changed(); }
      void lra_value_change(const semitone::var &) override { something_changed(); }
      void ov_value_change(const semitone::var &) override { something_changed(); }

    protected:
      consumable_resource &cr;
    };

  private:
    std::vector<riddle::id_token> ctr_ins;
    std::vector<std::vector<std::unique_ptr<const riddle::ast::expression>>> ctr_ivs;
    std::vector<std::unique_ptr<const riddle::ast::statement>> ctr_stmnts;
    ratio::core::predicate *p_pred;
    ratio::core::predicate *c_pred;
    std::vector<std::unique_ptr<const riddle::ast::statement>> pred_stmnts;
    std::set<const ratio::core::item *> to_check;             // the consumable-resource instances whose atoms have changed..
    std::vector<ratio::core::atom *> atoms;                   // we store, for each atom, its atom listener..
    std::vector<std::unique_ptr<cr_atom_listener>> listeners; // we store, for each atom, its atom listener..
  };
} // namespace ratio::solver
