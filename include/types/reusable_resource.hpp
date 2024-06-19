#pragma once

#include "smart_type.hpp"
#include "timeline.hpp"
#include "flaw.hpp"
#include "resolver.hpp"

namespace ratio
{
  class reusable_resource final : public smart_type, public timeline
  {
    class rr_atom_listener;
    friend class rr_atom_listener;

  public:
    reusable_resource(solver &slv);

  private:
    std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() noexcept override;

    void new_atom(std::shared_ptr<ratio::atom> &atm) noexcept override;

#ifdef ENABLE_VISUALIZATION
    json::json extract() const noexcept override;
#endif

    /**
     * @brief Represents a reusable resource flaw.
     *
     * A reusable resource flaw is a set two or more atoms that temporally overlap in the same reusable resource instance.
     *
     * This class inherits from the `flaw` class and represents a flaw related to a reusable resource.
     * It provides functionality to compute resolvers for the flaw.
     */
    class rr_flaw final : public flaw
    {
    public:
      rr_flaw(reusable_resource &rr, const std::set<atom *> &mcs);

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
         * @param flw The reference to the `rr_flaw` object.
         * @param r The reference to the `lit` object.
         * @param before The reference to the `atom` object that should be ordered before the other.
         * @param after The reference to the `atom` object that should be ordered after the other.
         */
        order_resolver(rr_flaw &flw, const utils::lit &r, const atom &before, const atom &after);

#ifdef ENABLE_VISUALIZATION
        json::json get_data() const noexcept override;
#endif

      private:
        void apply() override {}

      private:
        const atom &before; // applying the resolver will order this atom before the other..
        const atom &after;  // applying the resolver will order this atom after the other..
      };

      /**
       * @brief A resolver that forbids a specific atom on a given reusable resource.
       *
       * This resolver is used to forbid a specific atom on a given reusable resource in the solver.
       * It is a subclass of the base class `resolver`.
       */
      class forbid_resolver final : public resolver
      {
      public:
        /**
         * @brief Constructs a forbid_resolver object.
         *
         * @param flw The reference to the rr_flaw object.
         * @param r The reference to the utils::lit object.
         * @param atm The reference to the atom object.
         * @param itm The reference to the riddle::component object.
         */
        forbid_resolver(rr_flaw &flw, const utils::lit &r, atom &atm, riddle::component &itm);

#ifdef ENABLE_VISUALIZATION
        json::json get_data() const noexcept override;
#endif

      private:
        void apply() override {}

      private:
        atom &atm;              // applying the resolver will forbid this atom on the `itm` reusable resource..
        riddle::component &itm; // applying the resolver will forbid the `atm` atom on this reusable resource..
      };

#ifdef ENABLE_VISUALIZATION
      json::json get_data() const noexcept override;
#endif

    private:
      reusable_resource &rr;
      const std::set<atom *> mcs;
    };

    class rr_atom_listener final : private atom_listener
    {
    public:
      rr_atom_listener(reusable_resource &rr, atom &a);

      void something_changed();

      void on_sat_value_changed(VARIABLE_TYPE) override { something_changed(); }
      void on_rdl_value_changed(VARIABLE_TYPE) override { something_changed(); }
      void on_lra_value_changed(VARIABLE_TYPE) override { something_changed(); }
      void on_ov_value_changed(VARIABLE_TYPE) override { something_changed(); }

      reusable_resource &rr;
    };

  private:
    std::set<const riddle::item *> to_check;                        // the reusable-resource instances whose atoms have changed..
    std::vector<std::reference_wrapper<atom>> atoms;                // the atoms of the reusable-resource..
    std::set<std::set<atom *>> rr_flaws;                            // the reusable-resource flaws found so far..
    std::vector<std::unique_ptr<rr_atom_listener>> listeners;       // we store, for each atom, its atom listener..
    std::map<atom *, std::map<atom *, utils::lit>> leqs;            // all the possible ordering constraints..
    std::map<atom *, std::map<utils::enum_val *, utils::lit>> frbs; // all the possible forbidding constraints..
  };

#ifdef ENABLE_VISUALIZATION
  const json::json reusable_resource_timeline_value_schema{{"reusable_resource_timeline_value", {{"type", "object"}, {"properties", {{"from", {{"$ref", "#/components/schemas/inf_rational"}}}, {"to", {{"$ref", "#/components/schemas/inf_rational"}}}, {"usage", {{"$ref", "#/components/schemas/inf_rational"}}}, {"atoms", {{"type", "array"}, {"items", {{"type", "integer"}}}}}}}, {"required", {"from", "to", "usage"}}}}};
  const json::json reusable_resource_timeline_schema{{"reusable_resource_timeline", {{"type", "object"}, {"properties", {{"id", {{"type", "integer"}}}, {"type", {{"type", "string"}, {"enum", {"ReusableResource"}}}}, {"name", {{"type", "string"}}}, {"capacity", {{"$ref", "#/components/schemas/inf_rational"}}}, {"values", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/reusable_resource_timeline_value"}}}}}}}, {"required", {"id", "type", "name", "capacity"}}}}};
#endif
} // namespace ratio
