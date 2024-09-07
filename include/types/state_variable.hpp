#pragma once

#include "smart_type.hpp"
#include "timeline.hpp"
#include "flaw.hpp"
#include "resolver.hpp"

namespace ratio
{
  constexpr std::string_view STATE_VARIABLE_TYPE_NAME = "StateVariable";

  class state_variable final : public smart_type, public timeline
  {
    class sv_atom_listener;
    friend class sv_atom_listener;

  public:
    state_variable(solver &slv);

  private:
    void new_predicate(riddle::predicate &pred) override;

    std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() noexcept override;

    void new_atom(std::shared_ptr<ratio::atom> &atm) noexcept override;

#ifdef ENABLE_API
    json::json extract() const noexcept override;
#endif

    /**
     * @brief Represents a state variable flaw.
     *
     * A state variable flaw is a set two or more atoms that temporally overlap in the same state variable instance.
     *
     * This class inherits from the `flaw` class and represents a flaw related to a state variable.
     * It provides functionality to compute resolvers for the flaw.
     */
    class sv_flaw final : public flaw
    {
    public:
      sv_flaw(state_variable &sv, const std::set<atom *> &mcs);

    private:
      void compute_resolvers() override;

      /**
       * @brief Represents a resolver that orders two atoms.
       *
       * The `order_resolver` class is a concrete implementation of the `resolver` base class.
       * It is used to specify the order between two atoms, where applying the resolver will order
       * the `before` atom before the `after` atom.
       */
      class order_resolver final : public resolver
      {
      public:
        /**
         * @brief Constructs an `order_resolver` object.
         *
         * @param flw The reference to the `sv_flaw` object.
         * @param r The reference to the `lit` object.
         * @param before The reference to the `atom` object that should be ordered before the other.
         * @param after The reference to the `atom` object that should be ordered after the other.
         */
        order_resolver(sv_flaw &flw, const utils::lit &r, const atom &before, const atom &after);

#ifdef ENABLE_API
        json::json get_data() const noexcept override;
#endif

      private:
        void apply() override {}

      private:
        const atom &before; // applying the resolver will order this atom before the other..
        const atom &after;  // applying the resolver will order this atom after the other..
      };

      /**
       * @brief A resolver that forbids a specific atom on a given state variable.
       *
       * This resolver is used to forbid a specific atom on a given state variable in the solver.
       * It is a subclass of the base class `resolver`.
       */
      class forbid_resolver final : public resolver
      {
      public:
        /**
         * @brief Constructs a forbid_resolver object.
         *
         * @param flw The reference to the sv_flaw object.
         * @param r The reference to the utils::lit object.
         * @param atm The reference to the atom object.
         * @param itm The reference to the riddle::component object.
         */
        forbid_resolver(sv_flaw &flw, const utils::lit &r, atom &atm, riddle::component &itm);

#ifdef ENABLE_API
        json::json get_data() const noexcept override;
#endif

      private:
        void apply() override {}

      private:
        atom &atm;              // applying the resolver will forbid this atom on the `itm` state variable..
        riddle::component &itm; // applying the resolver will forbid the `atm` atom on this state variable..
      };

#ifdef ENABLE_API
      json::json get_data() const noexcept override;
#endif

    private:
      state_variable &sv;
      const std::set<atom *> mcs;
    };

    class sv_atom_listener final : private atom_listener
    {
    public:
      sv_atom_listener(state_variable &sv, atom &a);

      void something_changed();

      void on_sat_value_changed(VARIABLE_TYPE) override { something_changed(); }
      void on_rdl_value_changed(VARIABLE_TYPE) override { something_changed(); }
      void on_lra_value_changed(VARIABLE_TYPE) override { something_changed(); }
      void on_ov_value_changed(VARIABLE_TYPE) override { something_changed(); }

      state_variable &sv;
    };

  private:
    std::set<const riddle::item *> to_check;                        // the state-variable instances whose atoms have changed..
    std::vector<std::reference_wrapper<atom>> atoms;                // the atoms of the state-variable..
    std::set<std::set<atom *>> sv_flaws;                            // the state-variable flaws found so far..
    std::vector<std::unique_ptr<sv_atom_listener>> listeners;       // we store, for each atom, its atom listener..
    std::map<atom *, std::map<atom *, utils::lit>> leqs;            // all the possible ordering constraints..
    std::map<atom *, std::map<utils::enum_val *, utils::lit>> frbs; // all the possible forbidding constraints..
  };
} // namespace ratio
