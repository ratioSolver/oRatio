#pragma once

#include "smart_type.hpp"

namespace ratio
{
  class state_variable final : public smart_type, public timeline
  {
    class sv_atom_listener;
    friend class sv_atom_listener;

  public:
    state_variable(solver &slv);

  private:
    std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() const noexcept override { return {}; }

    void new_atom(std::shared_ptr<ratio::atom> &atm) noexcept override;

#ifdef ENABLE_VISUALIZATION
    json::json extract() const noexcept override;
#endif

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
    std::vector<std::unique_ptr<sv_atom_listener>> listeners;       // we store, for each atom, its atom listener..
    std::map<atom *, std::map<atom *, utils::lit>> leqs;            // all the possible ordering constraints..
    std::map<atom *, std::map<utils::enum_val *, utils::lit>> frbs; // all the possible forbidding constraints..
  };
} // namespace ratio
