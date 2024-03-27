#pragma once

#include "type.hpp"
#include "lit.hpp"

namespace ratio
{
  class solver;
  class atom;

  class smart_type : public riddle::component_type
  {
    friend class solver;

  public:
    smart_type(scope &parent, const std::string &name);
    virtual ~smart_type() = default;

    solver &get_solver() const { return slv; }

  private:
    /**
     * Returns all the decisions to take for solving the current inconsistencies with their choices' estimated costs.
     *
     * @return a vector of decisions, each represented by a vector of pairs containing the literals representing the choice and its estimated cost.
     */
    virtual std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() = 0;

    /**
     * @brief Notifies the smart type that a new atom has been created.
     *
     * @param atm The new atom that has been created.
     */
    virtual void new_atom(atom &atm) = 0;

  private:
    solver &slv;
  };
} // namespace ratio
