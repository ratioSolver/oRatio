#pragma once

#include "smart_type.hpp"

namespace ratio
{
  class reusable_resource final : public smart_type, public timeline
  {
    class rr_atom_listener;
    friend class rr_atom_listener;

  public:
    reusable_resource(solver &slv);

  private:
    std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() const noexcept override { return {}; }

    void new_atom(std::shared_ptr<ratio::atom> &atm) noexcept override;

#ifdef ENABLE_VISUALIZATION
    json::json extract() const noexcept override;
#endif

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
    std::vector<std::unique_ptr<rr_atom_listener>> listeners;       // we store, for each atom, its atom listener..
    std::map<atom *, std::map<atom *, utils::lit>> leqs;            // all the possible ordering constraints..
    std::map<atom *, std::map<utils::enum_val *, utils::lit>> frbs; // all the possible forbidding constraints..
  };
} // namespace ratio
