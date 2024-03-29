#pragma once

#include "smart_type.h"
#include "flaw.h"
#include "resolver.h"

#define STATE_VARIABLE_NAME "StateVariable"

namespace ratio
{
  class state_variable final : public smart_type, public timeline
  {
  public:
    state_variable(riddle::scope &scp);

  private:
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() override;

    void new_predicate(riddle::predicate &pred) noexcept override;

    void new_atom(atom &atm) override;

    class sv_constructor final : public riddle::constructor
    {
    public:
      sv_constructor(state_variable &sv) : riddle::constructor(sv, {}, {}, {}, {}) {}
    };

    class sv_atom_listener final : public atom_listener
    {
    public:
      sv_atom_listener(state_variable &sv, atom &a) : atom_listener(a), sv(sv) {}

    private:
      void something_changed();

      void sat_value_change(const semitone::var &) override { something_changed(); }
      void rdl_value_change(const semitone::var &) override { something_changed(); }
      void lra_value_change(const semitone::var &) override { something_changed(); }
      void ov_value_change(const semitone::var &) override { something_changed(); }

    protected:
      state_variable &sv;
    };

    using sv_atom_listener_ptr = utils::u_ptr<sv_atom_listener>;

    // the flaw (i.e. two or more temporally overlapping atoms on the same state-variable instance) that can arise from a state-variable..
    class sv_flaw final : public flaw
    {
      friend class state_variable;

    public:
      sv_flaw(state_variable &sv, const std::set<atom *> &atms);

      json::json get_data() const noexcept override;

    private:
      void compute_resolvers() override;

      // a resolver for temporally ordering atoms..
      class order_resolver final : public resolver
      {
      public:
        order_resolver(sv_flaw &flw, const semitone::lit &r, const atom &before, const atom &after);

        json::json get_data() const noexcept override;

      private:
        void apply() override {}

      private:
        const atom &before; // applying the resolver will order this atom before the other..
        const atom &after;  // applying the resolver will order this atom after the other..
      };

      // a resolver for forbidding atoms on a specific state-variable..
      class forbid_resolver final : public resolver
      {
      public:
        forbid_resolver(sv_flaw &flw, const semitone::lit &r, atom &atm, riddle::item &itm);

        json::json get_data() const noexcept override;

      private:
        void apply() override {}

      private:
        atom &atm;         // applying the resolver will forbid this atom on the `itm` item..
        riddle::item &itm; // applying the resolver will forbid the `atm` atom on this item..
      };

    private:
      state_variable &sv;
      const std::set<atom *> overlapping_atoms;
    };

    json::json extract() const noexcept override;

  private:
    std::set<const riddle::item *> to_check;     // the state-variable instances whose atoms have changed..
    std::vector<atom *> atoms;                   // we store, for each atom, its atom listener..
    std::vector<sv_atom_listener_ptr> listeners; // we store, for each atom, its atom listener..

    std::map<std::set<atom *>, sv_flaw *> sv_flaws;                 // the state-variable flaws found so far..
    std::map<atom *, std::map<atom *, semitone::lit>> leqs;         // all the possible ordering constraints..
    std::map<atom *, std::map<riddle::item *, semitone::lit>> frbs; // all the possible forbidding constraints..
  };
} // namespace ratio
