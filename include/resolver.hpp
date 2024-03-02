#pragma once

#include <cstdint>

namespace ratio
{
  class solver;

  class resolver
  {
  private:
    solver &s; // the solver this flaw belongs to..
  };

  /**
   * @brief Gets the id of the given resolver.
   *
   * @param r the resolver to get the id of.
   * @return uintptr_t the id of the given resolver.
   */
  inline uintptr_t get_id(const resolver &r) noexcept { return reinterpret_cast<uintptr_t>(&r); }
} // namespace ratio
