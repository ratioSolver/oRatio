#pragma once

#include "smart_type.h"
#include "flaw.h"
#include "resolver.h"

#define CONSUMABLE_RESOURCE_NAME "ConsumableResource"
#define CONSUMABLE_RESOURCE_INITIAL_AMOUNT "initial_amount"
#define CONSUMABLE_RESOURCE_CAPACITY "capacity"
#define CONSUMABLE_RESOURCE_PRODUCE_PREDICATE_NAME "Produce"
#define CONSUMABLE_RESOURCE_CONSUME_PREDICATE_NAME "Consume"
#define CONSUMABLE_RESOURCE_AMOUNT_NAME "amount"

namespace ratio
{
  class consumable_resource final : public smart_type, public timeline
  {
  public:
    consumable_resource(riddle::scope &scp);

    const riddle::predicate &get_produce_predicate() const noexcept { return *p_pred; }
    const riddle::predicate &get_consume_predicate() const noexcept { return *c_pred; }

  private:
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() override;

    void new_atom(atom &atm) override;

    class cr_atom_listener final : public atom_listener
    {
    public:
      cr_atom_listener(consumable_resource &cr, atom &a) : atom_listener(a), cr(cr) {}

    private:
      void something_changed();

      void sat_value_change(const semitone::var &) override { something_changed(); }
      void rdl_value_change(const semitone::var &) override { something_changed(); }
      void lra_value_change(const semitone::var &) override { something_changed(); }
      void ov_value_change(const semitone::var &) override { something_changed(); }

    protected:
      consumable_resource &cr;
    };

    using cr_atom_listener_ptr = utils::u_ptr<cr_atom_listener>;

    // the flaw (i.e. two or more temporally overlapping atoms on the same reusable-resource instance) that can arise from a reusable-resource..
    class cr_flaw final : public flaw
    {
      friend class consumable_resource;

    public:
      cr_flaw(consumable_resource &cr, const std::set<atom *> &atms);

      json::json get_data() const noexcept override;

    private:
      void compute_resolvers() override;

      // a resolver for temporally ordering atoms..
      class order_resolver final : public resolver
      {
      public:
        order_resolver(cr_flaw &flw, const semitone::lit &r, const atom &before, const atom &after);

        json::json get_data() const noexcept override;

      private:
        void apply() override {}

      private:
        const atom &before; // applying the resolver will order this atom before the other..
        const atom &after;  // applying the resolver will order this atom after the other..
      };

      // a resolver for forbidding atoms on a specific reusable-resource..
      class forbid_resolver final : public resolver
      {
      public:
        forbid_resolver(cr_flaw &flw, atom &atm, riddle::item &itm);

        json::json get_data() const noexcept override;

      private:
        void apply() override {}

      private:
        atom &atm;         // applying the resolver will forbid this atom on the `itm` item..
        riddle::item &itm; // applying the resolver will forbid the `atm` atom on this item..
      };

    private:
      consumable_resource &cr;
      const std::set<atom *> overlapping_atoms;
    };

    json::json extract() const noexcept override;

  private:
    std::vector<riddle::id_token> ctr_ins;
    std::vector<std::vector<riddle::ast::expression_ptr>> ctr_ivs;
    std::vector<riddle::ast::statement_ptr> ctr_body;
    riddle::predicate *p_pred = nullptr;
    riddle::predicate *c_pred = nullptr;
    std::vector<riddle::ast::statement_ptr> pred_body;

  private:
    std::set<const riddle::item *> to_check;     // the reusable-resource instances whose atoms have changed..
    std::vector<atom *> atoms;                   // we store, for each atom, its atom listener..
    std::vector<cr_atom_listener_ptr> listeners; // we store, for each atom, its atom listener..

    std::map<std::set<atom *>, cr_flaw *> cr_flaws;                 // the reusable-resource flaws found so far..
    std::map<atom *, std::map<atom *, semitone::lit>> leqs;         // all the possible ordering constraints..
    std::map<atom *, std::map<riddle::item *, semitone::lit>> frbs; // all the possible forbidding constraints..
  };
} // namespace ratio
