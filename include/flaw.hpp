#pragma once

#include <cstdint>

namespace ratio
{
  class solver;

  class flaw
  {
  private:
    solver &s; // the solver this flaw belongs to..
  };

  /**
   * @brief Gets the id of the given flaw.
   *
   * @param f the flaw to get the id of.
   * @return uintptr_t the id of the given flaw.
   */
  inline uintptr_t get_id(const flaw &f) noexcept { return reinterpret_cast<uintptr_t>(&f); }
} // namespace ratio
