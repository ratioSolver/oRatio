#pragma once

#include <vector>
#include <cstdint>
#include "lit.hpp"
#include "rational.hpp"

namespace ratio
{
  class solver;
  class resolver;

  class flaw
  {
  public:
    flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, const bool &exclusive = false) noexcept;

    solver &get_solver() noexcept { return s; }
    const solver &get_solver() const noexcept { return s; }

  private:
    solver &s;                                                     // the solver this flaw belongs to..
    std::vector<std::reference_wrapper<resolver>> causes;          // the resolvers that caused this flaw..
    bool exclusive;                                                // whether this flaw is exclusive..
    utils::lit phi;                                                // the literal indicating whether the flaw is active or not (this literal is initialized by the `init` procedure)..
    utils::rational est_cost = utils::rational::positive_infinite; // the current estimated cost of the flaw..
    VARIABLE_TYPE position;                                        // the position variable (i.e., an integer time-point) associated to this flaw..
    bool expanded = false;                                         // whether this flaw has been expanded or not..
    std::vector<std::reference_wrapper<resolver>> resolvers;       // the resolvers for this flaw..
    std::vector<std::reference_wrapper<resolver>> supports;        // the resolvers supported by this flaw (used for propagating cost estimates)..
  };

  /**
   * @brief Gets the id of the given flaw.
   *
   * @param f the flaw to get the id of.
   * @return uintptr_t the id of the given flaw.
   */
  inline uintptr_t get_id(const flaw &f) noexcept { return reinterpret_cast<uintptr_t>(&f); }
} // namespace ratio
