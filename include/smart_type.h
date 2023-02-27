#pragma once

#include "type.h"
#include "sat_value_listener.h"
#include "lra_value_listener.h"
#include "rdl_value_listener.h"
#include "ov_value_listener.h"

namespace ratio
{
  class solver;
  class atom;

  class smart_type : public riddle::complex_type
  {
    friend class solver;

  public:
    smart_type(riddle::scope &scp, const std::string &name);
    virtual ~smart_type() = default;

    inline solver &get_solver() const noexcept { return slv; }

  private:
    /**
     * Returns all the decisions to take for solving the current inconsistencies with their choices' estimated costs.
     *
     * @return a vector of decisions, each represented by a vector of pairs containing the literals representing the choice and its estimated cost.
     */
    virtual std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() = 0;

    /**
     * @brief Notifies the smart type that a new atom has been created.
     *
     * @param atm The new atom that has been created.
     */
    virtual void new_atom(atom &atm) = 0;

  protected:
    void set_ni(const semitone::lit &v) noexcept; // temporally sets the solver's `ni` literal..
    void restore_ni() noexcept;                   // restores the solver's `ni` literal..

  private:
    solver &slv;
  };

  class atom_listener : public semitone::sat_value_listener, public semitone::lra_value_listener, public semitone::rdl_value_listener, public semitone::ov_value_listener
  {
  public:
    atom_listener(atom &atm);
    virtual ~atom_listener() = default;

  protected:
    atom &atm;
  };

  class timeline
  {
  public:
    virtual ~timeline() = default;

    virtual json::json extract() const noexcept = 0;
  };
} // namespace ratio
