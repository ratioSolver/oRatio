#pragma once

#include <cstdint>
#include <vector>
#include "rational.hpp"
#include "lit.hpp"

namespace ratio
{
  class solver;
  class flaw;

  class resolver
  {
  public:
    resolver(flaw &f, const utils::rational &intrinsic_cost);
    resolver(flaw &f, const utils::lit &rho, const utils::rational &intrinsic_cost);
    virtual ~resolver() = default;

    flaw &get_flaw() noexcept { return f; }
    const flaw &get_flaw() const noexcept { return f; }
    const utils::lit &get_rho() const noexcept { return rho; }
    const utils::rational &get_intrinsic_cost() const noexcept { return intrinsic_cost; }

  private:
    flaw &f;                                                 // the flaw solved by this resolver..
    const utils::lit rho;                                    // the propositional literal indicating whether the resolver is active or not..
    const utils::rational intrinsic_cost;                    // the intrinsic cost of the resolver..
    std::vector<std::reference_wrapper<flaw>> preconditions; // the preconditions of this resolver..
  };

  /**
   * @brief Gets the id of the given resolver.
   *
   * @param r the resolver to get the id of.
   * @return uintptr_t the id of the given resolver.
   */
  inline uintptr_t get_id(const resolver &r) noexcept { return reinterpret_cast<uintptr_t>(&r); }
} // namespace ratio
