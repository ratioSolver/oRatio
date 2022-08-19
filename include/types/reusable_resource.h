#pragma once

#include "smart_type.h"
#include "constructor.h"
#include "predicate.h"
#include "flaw.h"
#include "resolver.h"

#define REUSABLE_RESOURCE_NAME "ReusableResource"
#define REUSABLE_RESOURCE_CAPACITY "capacity"
#define REUSABLE_RESOURCE_USE_PREDICATE_NAME "Use"
#define REUSABLE_RESOURCE_USE_AMOUNT_NAME "amount"

namespace ratio::solver
{
  class reusable_resource final : public smart_type
  {
  public:
    reusable_resource(solver &slv);
    reusable_resource(const reusable_resource &orig) = delete;

    const std::vector<ratio::core::atom *> &get_atoms() const noexcept { return atoms; }
    const ratio::core::type &get_use_predicate() const noexcept { return *u_pred; }

  private:
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() override;

    void new_predicate(ratio::core::predicate &) noexcept override;
    void new_atom_flaw(atom_flaw &f) override;
    void store_variables(ratio::core::atom &atm0, ratio::core::atom &atm1);

    // the reusable-resource constructor..
    class rr_constructor final : public ratio::core::constructor
    {
    public:
      rr_constructor(reusable_resource &rr, std::vector<ratio::core::field_ptr> args, const std::vector<riddle::id_token> &ins, const std::vector<std::vector<std::unique_ptr<const riddle::ast::expression>>> &ivs, const std::vector<std::unique_ptr<const riddle::ast::statement>> &stmnts);
      rr_constructor(rr_constructor &&) = delete;
    };

    // the reusable-resource `Use` predicate..
    class use_predicate final : public ratio::core::predicate
    {
    public:
      use_predicate(reusable_resource &rr, std::vector<ratio::core::field_ptr> args, const std::vector<std::unique_ptr<const riddle::ast::statement>> &stmnts);
      use_predicate(use_predicate &&) = delete;
    };

    // the atom listener for the reusable-resource..
    class rr_atom_listener final : public atom_listener
    {
    public:
      rr_atom_listener(reusable_resource &rr, ratio::core::atom &atm);
      rr_atom_listener(rr_atom_listener &&) = delete;

    private:
      void something_changed();

      void sat_value_change(const semitone::var &) override { something_changed(); }
      void rdl_value_change(const semitone::var &) override { something_changed(); }
      void lra_value_change(const semitone::var &) override { something_changed(); }
      void ov_value_change(const semitone::var &) override { something_changed(); }

    protected:
      reusable_resource &rr;
    };

    // the flaw (i.e. two or more temporally overlapping atoms on the same reusable-resource instance) that can arise from a reusable-resource..
    class rr_flaw final : public flaw
    {
      friend class state_variable;

    public:
      rr_flaw(reusable_resource &rr, const std::set<ratio::core::atom *> &atms);
      rr_flaw(rr_flaw &&) = delete;

      ORATIO_EXPORT std::string get_data() const noexcept override;

    private:
      void compute_resolvers() override;

    private:
      reusable_resource &rr;
      const std::set<ratio::core::atom *> overlapping_atoms;
    };

    // a resolver for temporally ordering atoms..
    class order_resolver final : public resolver
    {
    public:
      order_resolver(rr_flaw &flw, const semitone::lit &r, const ratio::core::atom &before, const ratio::core::atom &after);
      order_resolver(const order_resolver &that) = delete;

      ORATIO_EXPORT std::string get_data() const noexcept override;

    private:
      void apply() override;

    private:
      const ratio::core::atom &before; // applying the resolver will order this atom before the other..
      const ratio::core::atom &after;  // applying the resolver will order this atom after the other..
    };

    // a resolver for placing atoms on a specific reusable-resource..
    class place_resolver final : public resolver
    {
    public:
      place_resolver(rr_flaw &flw, const semitone::lit &r, ratio::core::atom &plc_atm, const ratio::core::item &plc_itm, ratio::core::atom &frbd_atm);
      place_resolver(const place_resolver &that) = delete;

      ORATIO_EXPORT std::string get_data() const noexcept override;
      const ratio::core::item &get_place_item() const noexcept { return plc_itm; }

    private:
      void apply() override;

    private:
      ratio::core::atom &plc_atm;       // applying the resolver will force this atom on the `plc_item` item..
      const ratio::core::item &plc_itm; // applying the resolver will force the `plc_atm` atom on this item..
      ratio::core::atom &frbd_atm;      // applying the resolver will forbid this atom on the `plc_itm` item..
    };

    // a resolver for forbidding atoms on a specific reusable-resource..
    class forbid_resolver final : public resolver
    {
    public:
      forbid_resolver(rr_flaw &flw, ratio::core::atom &atm, ratio::core::item &itm);
      forbid_resolver(const forbid_resolver &that) = delete;

      ORATIO_EXPORT std::string get_data() const noexcept override;

    private:
      void apply() override;

    private:
      ratio::core::atom &atm; // applying the resolver will forbid this atom on the `itm` item..
      ratio::core::item &itm; // applying the resolver will forbid the `atm` atom on this item..
    };

  private:
    std::vector<riddle::id_token> ctr_ins;
    std::vector<std::vector<std::unique_ptr<const riddle::ast::expression>>> ctr_ivs;
    std::vector<std::unique_ptr<const riddle::ast::statement>> ctr_stmnts;
    ratio::core::predicate *u_pred = nullptr;
    std::vector<std::unique_ptr<const riddle::ast::statement>> pred_stmnts;
    std::set<const ratio::core::item *> to_check;             // the reusable-resource instances whose atoms have changed..
    std::vector<ratio::core::atom *> atoms;                   // we store, for each atom, its atom listener..
    std::vector<std::unique_ptr<rr_atom_listener>> listeners; // we store, for each atom, its atom listener..

    std::map<std::set<ratio::core::atom *>, rr_flaw *> rr_flaws;                                                    // the reusable-resource flaws found so far..
    std::map<ratio::core::atom *, std::map<ratio::core::atom *, semitone::lit>> leqs;                               // all the possible ordering constraints..
    std::map<std::set<ratio::core::atom *>, std::vector<std::pair<semitone::lit, const ratio::core::item *>>> plcs; // all the possible placement constraints..
  };
} // namespace ratio::solver