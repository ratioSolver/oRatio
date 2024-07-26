#pragma once

#include "smart_type.hpp"
#include "timeline.hpp"

namespace ratio
{
  class consumable_resource final : public smart_type, public timeline
  {
    class cr_atom_listener;
    friend class cr_atom_listener;

  public:
    consumable_resource(solver &slv);

  private:
    std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() noexcept override { return {}; }

    void new_atom(std::shared_ptr<ratio::atom> &atm) noexcept override;

#ifdef ENABLE_API
    json::json extract() const noexcept override;
#endif

    class cr_atom_listener final : private atom_listener
    {
    public:
      cr_atom_listener(consumable_resource &cr, atom &a);

      void something_changed();

      void on_sat_value_changed(VARIABLE_TYPE) override { something_changed(); }
      void on_rdl_value_changed(VARIABLE_TYPE) override { something_changed(); }
      void on_lra_value_changed(VARIABLE_TYPE) override { something_changed(); }
      void on_ov_value_changed(VARIABLE_TYPE) override { something_changed(); }

      consumable_resource &cr;
    };

  private:
    std::set<const riddle::item *> to_check;                        // the consumable-resource instances whose atoms have changed..
    std::vector<std::reference_wrapper<atom>> atoms;                // the atoms of the consumable-resource..
    std::vector<std::unique_ptr<cr_atom_listener>> listeners;       // we store, for each atom, its atom listener..
    std::map<atom *, std::map<atom *, utils::lit>> leqs;            // all the possible ordering constraints..
    std::map<atom *, std::map<utils::enum_val *, utils::lit>> frbs; // all the possible forbidding constraints..
  };
} // namespace ratio
