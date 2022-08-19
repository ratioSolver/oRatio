#pragma once

#include "item.h"
#include "type.h"
#include "sat_value_listener.h"
#include "lra_value_listener.h"
#include "rdl_value_listener.h"
#include "ov_value_listener.h"

namespace ratio::solver
{
  class solver;
  class flaw;
  class atom_flaw;
  class resolver;

  class smart_type : public ratio::core::type
  {
    friend class solver;

  public:
    smart_type(scope &scp, const std::string &name);
    smart_type(const smart_type &that) = delete;
    virtual ~smart_type() = default;

    inline solver &get_solver() const noexcept { return slv; }

  private:
    /**
     * Returns all the decisions to take for solving the current inconsistencies with their choices' estimated costs.
     *
     * @return a vector of decisions, each represented by a vector of pairs containing the literals representing the choice and its estimated cost.
     */
    virtual std::vector<std::vector<std::pair<semitone::lit, double>>> get_current_incs() = 0;

    virtual void new_atom_flaw(atom_flaw &);

  protected:
    void set_ni(const semitone::lit &v) noexcept; // temporally sets the solver's `ni` literal..
    void restore_ni() noexcept;                   // restores the solver's `ni` literal..

    void store_flaw(std::unique_ptr<flaw> f) noexcept; // stores the flaw waiting for its initialization at root-level..

    static std::vector<resolver *> get_resolvers(solver &slv, const std::set<ratio::core::atom *> &atms) noexcept; // returns the vector of resolvers which has given rise to the given atoms..

  private:
    solver &slv;
  };

  class atom_listener : public semitone::sat_value_listener, public semitone::lra_value_listener, public semitone::rdl_value_listener, public semitone::ov_value_listener
  {
  public:
    atom_listener(ratio::core::atom &atm);
    atom_listener(const atom_listener &that) = delete;
    virtual ~atom_listener() = default;

  protected:
    ratio::core::atom &atm;
  };
} // namespace ratio::solver