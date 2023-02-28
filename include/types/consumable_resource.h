#pragma once

#include "smart_type.h"
#include "flaw.h"
#include "resolver.h"

#define CONSUMABLE_RESOURCE_NAME "ConsumableResource"
#define CONSUMABLE_RESOURCE_INITIAL_AMOUNT "initial_amount"
#define CONSUMABLE_RESOURCE_CAPACITY "capacity"
#define CONSUMABLE_RESOURCE_PRODUCE_PREDICATE_NAME "Produce"
#define CONSUMABLE_RESOURCE_CONSUME_PREDICATE_NAME "Consume"
#define CONSUMABLE_RESOURCE_USE_AMOUNT_NAME "amount"

namespace ratio
{
  class consumable_resource final : public smart_type
  {
  public:
    consumable_resource(riddle::scope &scp);

  private:
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() override { return {}; }

    void new_atom(atom &atm) override;

    void store_variables(atom &atm0, atom &atm1);

    class cr_constructor final : public riddle::constructor
    {
    public:
      cr_constructor(consumable_resource &cr) : riddle::constructor(cr, {}, {}) {}
    };

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
        void apply() override;

      private:
        const atom &before; // applying the resolver will order this atom before the other..
        const atom &after;  // applying the resolver will order this atom after the other..
      };

      // a resolver for placing atoms on a specific reusable-resource..
      class place_resolver final : public resolver
      {
      public:
        place_resolver(cr_flaw &flw, const semitone::lit &r, atom &plc_atm, const riddle::item &plc_itm, atom &frbd_atm);

        json::json get_data() const noexcept override;
        const riddle::item &get_place_item() const noexcept { return plc_itm; }

      private:
        void apply() override;

      private:
        atom &plc_atm;               // applying the resolver will force this atom on the `plc_item` item..
        const riddle::item &plc_itm; // applying the resolver will force the `plc_atm` atom on this item..
        atom &frbd_atm;              // applying the resolver will forbid this atom on the `plc_itm` item..
      };

      // a resolver for forbidding atoms on a specific reusable-resource..
      class forbid_resolver final : public resolver
      {
      public:
        forbid_resolver(cr_flaw &flw, atom &atm, riddle::item &itm);

        json::json get_data() const noexcept override;

      private:
        void apply() override;

      private:
        atom &atm;         // applying the resolver will forbid this atom on the `itm` item..
        riddle::item &itm; // applying the resolver will forbid the `atm` atom on this item..
      };

    private:
      consumable_resource &cr;
      const std::set<atom *> overlapping_atoms;
    };

  private:
    std::set<const riddle::item *> to_check;     // the reusable-resource instances whose atoms have changed..
    std::vector<atom *> atoms;                   // we store, for each atom, its atom listener..
    std::vector<cr_atom_listener_ptr> listeners; // we store, for each atom, its atom listener..

    std::map<std::set<atom *>, cr_flaw *> cr_flaws;                                               // the reusable-resource flaws found so far..
    std::map<atom *, std::map<atom *, semitone::lit>> leqs;                                       // all the possible ordering constraints..
    std::map<std::set<atom *>, std::vector<std::pair<semitone::lit, const riddle::item *>>> plcs; // all the possible placement constraints..
  };
} // namespace ratio
