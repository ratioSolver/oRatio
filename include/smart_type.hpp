#pragma once

#include "type.hpp"
#include "sat_value_listener.hpp"
#include "lra_value_listener.hpp"
#include "rdl_value_listener.hpp"
#include "ov_value_listener.hpp"

namespace ratio
{
  class solver;
  class atom;

  /**
   * @brief The smart_type class represents a type of component that can be used in the solver.
   * It inherits from the riddle::component_type class.
   */
  class smart_type : public riddle::component_type
  {
    friend class solver;

  public:
    /**
     * @brief Constructs a smart_type object with the specified parent scope and name.
     *
     * @param parent The parent scope of the smart_type.
     * @param name The name of the smart_type.
     */
    smart_type(scope &parent, const std::string &name);

    /**
     * @brief Default destructor for the smart_type class.
     */
    virtual ~smart_type() = default;

    /**
     * @brief Gets the solver associated with the smart_type.
     *
     * @return A reference to the solver object.
     */
    solver &get_solver() const { return slv; }

  private:
    /**
     * @brief Returns all the decisions to take for solving the current inconsistencies with their choices' estimated costs.
     *
     * @return A vector of decisions, each represented by a vector of pairs containing the literals representing the choice and its estimated cost.
     */
    virtual std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() = 0;

    /**
     * @brief Notifies the smart_type that a new atom has been created.
     *
     * @param atm The new atom that has been created.
     */
    virtual void new_atom(atom &atm) = 0;

  private:
    solver &slv;
  };

  /**
   * @brief Listener class for atom's parameters changes.
   *
   * This class inherits from semitone::lra_value_listener, semitone::rdl_value_listener, and semitone::ov_value_listener to provide a listener for different types of atom's parameters changes.
   */
  class atom_listener : private semitone::lra_value_listener, private semitone::rdl_value_listener, private semitone::ov_value_listener
  {
  public:
    /**
     * @brief Constructs an atom_listener object.
     *
     * @param atm The atom object to listen to.
     */
    atom_listener(riddle::atom &atm);

    /**
     * @brief Default destructor for the atom_listener class.
     */
    virtual ~atom_listener();

  private:
    riddle::atom &atm; // The atom object to listen to
  };
} // namespace ratio
