#pragma once

#include "smart_type.h"
#include "flaw.h"
#include "resolver.h"

#define REUSABLE_RESOURCE_NAME "ReusableResource"
#define REUSABLE_RESOURCE_CAPACITY "capacity"
#define REUSABLE_RESOURCE_USE_PREDICATE_NAME "Use"
#define REUSABLE_RESOURCE_AMOUNT_NAME "amount"

namespace ratio
{
  class reusable_resource final : public smart_type, public timeline
  {
  public:
    reusable_resource(riddle::scope &scp);

  private:
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() override;

    void new_atom(atom &atm) override;

    class rr_atom_listener final : public atom_listener
    {
    public:
      rr_atom_listener(reusable_resource &rr, atom &a) : atom_listener(a), rr(rr) {}

    private:
      void something_changed();

      void sat_value_change(const semitone::var &) override { something_changed(); }
      void rdl_value_change(const semitone::var &) override { something_changed(); }
      void lra_value_change(const semitone::var &) override { something_changed(); }
      void ov_value_change(const semitone::var &) override { something_changed(); }

    protected:
      reusable_resource &rr;
    };

    using rr_atom_listener_ptr = utils::u_ptr<rr_atom_listener>;

    // the flaw (i.e. two or more temporally overlapping atoms on the same reusable-resource instance) that can arise from a reusable-resource..
    class rr_flaw final : public flaw
    {
      friend class reusable_resource;

    public:
      rr_flaw(reusable_resource &rr, const std::set<atom *> &atms);

      json::json get_data() const noexcept override;

    private:
      void compute_resolvers() override;

      // a resolver for temporally ordering atoms..
      class order_resolver final : public resolver
      {
      public:
        order_resolver(rr_flaw &flw, const semitone::lit &r, const atom &before, const atom &after);

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
        forbid_resolver(rr_flaw &flw, const semitone::lit &r, atom &atm, riddle::item &itm);

        json::json get_data() const noexcept override;

      private:
        void apply() override {}

      private:
        atom &atm;         // applying the resolver will forbid this atom on the `itm` item..
        riddle::item &itm; // applying the resolver will forbid the `atm` atom on this item..
      };

    private:
      reusable_resource &rr;
      const std::set<atom *> overlapping_atoms;
    };

    json::json extract() const noexcept override;

  private:
    std::vector<riddle::id_token> ctr_ins;
    std::vector<std::vector<riddle::ast::expression_ptr>> ctr_ivs;
    std::vector<riddle::ast::statement_ptr> ctr_body;
    riddle::predicate *u_pred = nullptr;
    std::vector<riddle::ast::statement_ptr> pred_body;

  private:
    std::set<const riddle::item *> to_check;     // the reusable-resource instances whose atoms have changed..
    std::vector<atom *> atoms;                   // we store, for each atom, its atom listener..
    std::vector<rr_atom_listener_ptr> listeners; // we store, for each atom, its atom listener..

    std::map<std::set<atom *>, rr_flaw *> rr_flaws;                 // the reusable-resource flaws found so far..
    std::map<atom *, std::map<atom *, semitone::lit>> leqs;         // all the possible ordering constraints..
    std::map<atom *, std::map<riddle::item *, semitone::lit>> frbs; // all the possible forbidding constraints..
  };
} // namespace ratio
