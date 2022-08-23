#pragma once

#include "smart_type.h"
#include "constructor.h"
#include "flaw.h"
#include "resolver.h"

#define STATE_VARIABLE_NAME "StateVariable"

namespace ratio::solver
{
  class order_resolver;

  class state_variable final : public smart_type
  {
    friend class order_resolver;

  public:
    state_variable(solver &slv);
    state_variable(const state_variable &orig) = delete;

    const std::vector<ratio::core::atom *> &get_atoms() const noexcept { return atoms; }

  private:
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() override;

    void new_predicate(ratio::core::predicate &pred) noexcept override;
    void new_atom_flaw(atom_flaw &f) override;
    void store_variables(ratio::core::atom &atm0, ratio::core::atom &atm1);

    // the state-variable constructor..
    class sv_constructor final : public ratio::core::constructor
    {
    public:
      sv_constructor(state_variable &sv);
      sv_constructor(sv_constructor &&) = delete;
    };

    // the atom listener for the state-variable..
    class sv_atom_listener final : public atom_listener
    {
    public:
      sv_atom_listener(state_variable &sv, ratio::core::atom &atm);
      sv_atom_listener(sv_atom_listener &&) = delete;

    private:
      void something_changed();

      void sat_value_change(const semitone::var &) override { something_changed(); }
      void rdl_value_change(const semitone::var &) override { something_changed(); }
      void lra_value_change(const semitone::var &) override { something_changed(); }
      void ov_value_change(const semitone::var &) override { something_changed(); }

    protected:
      state_variable &sv;
    };

    // the flaw (i.e. two or more temporally overlapping atoms on the same state-variable instance) that can arise from a state-variable..
    class sv_flaw final : public flaw
    {
      friend class state_variable;

    public:
      sv_flaw(state_variable &sv, const std::set<ratio::core::atom *> &atms);
      sv_flaw(sv_flaw &&) = delete;

      ORATIO_EXPORT json::json get_data() const noexcept override;

    private:
      void compute_resolvers() override;

    private:
      state_variable &sv;
      const std::set<ratio::core::atom *> overlapping_atoms;
    };

    // a resolver for temporally ordering atoms..
    class order_resolver final : public resolver
    {
    public:
      order_resolver(sv_flaw &flw, const semitone::lit &r, const ratio::core::atom &before, const ratio::core::atom &after);
      order_resolver(const order_resolver &that) = delete;

      ORATIO_EXPORT json::json get_data() const noexcept override;

    private:
      void apply() override;

    private:
      const ratio::core::atom &before; // applying the resolver will order this atom before the other..
      const ratio::core::atom &after;  // applying the resolver will order this atom after the other..
    };

    // a resolver for placing atoms on a specific state-variable..
    class place_resolver final : public resolver
    {
    public:
      place_resolver(sv_flaw &flw, const semitone::lit &r, ratio::core::atom &plc_atm, const ratio::core::item &plc_itm, ratio::core::atom &frbd_atm);
      place_resolver(const place_resolver &that) = delete;

      ORATIO_EXPORT json::json get_data() const noexcept override;
      const ratio::core::item &get_place_item() const noexcept { return plc_itm; }

    private:
      void apply() override;

    private:
      ratio::core::atom &plc_atm;       // applying the resolver will force this atom on the `plc_item` item..
      const ratio::core::item &plc_itm; // applying the resolver will force the `plc_atm` atom on this item..
      ratio::core::atom &frbd_atm;      // applying the resolver will forbid this atom on the `plc_itm` item..
    };

    // a resolver for forbidding atoms on a specific state-variable..
    class forbid_resolver final : public resolver
    {
    public:
      forbid_resolver(sv_flaw &flw, ratio::core::atom &atm, ratio::core::item &itm);
      forbid_resolver(const forbid_resolver &that) = delete;

      ORATIO_EXPORT json::json get_data() const noexcept override;

    private:
      void apply() override;

    private:
      ratio::core::atom &atm; // applying the resolver will forbid this atom on the `itm` item..
      ratio::core::item &itm; // applying the resolver will forbid the `atm` atom on this item..
    };

    json::json extract() const noexcept;

  private:
    std::set<const ratio::core::item *> to_check;                   // the state-variable instances whose atoms have changed..
    std::vector<ratio::core::atom *> atoms;                         // we store, for each atom, its atom listener..
    std::vector<std::unique_ptr<const sv_atom_listener>> listeners; // we store, for each atom, its atom listener..

    std::map<std::set<ratio::core::atom *>, sv_flaw *> sv_flaws;                                                    // the state-variable flaws found so far..
    std::map<ratio::core::atom *, std::map<ratio::core::atom *, semitone::lit>> leqs;                               // all the possible ordering constraints..
    std::map<std::set<ratio::core::atom *>, std::vector<std::pair<semitone::lit, const ratio::core::item *>>> plcs; // all the possible placement constraints..
  };
} // namespace ratio::solver