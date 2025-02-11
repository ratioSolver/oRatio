#pragma once

#include "stsolver.hpp"

namespace ratio
{
  class stflaw : public flaw
  {
  public:
    stflaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes) noexcept;

    [[nodiscard]] const utils::lit &get_phi() const noexcept { return phi; }

    [[nodiscard]] const utils::var &get_pos() const noexcept { return pos; }

  private:
    [[nodiscard]] static utils::lit compute_phi(stsolver &slv, const std::vector<utils::ref_wrapper<resolver>> &causes) noexcept;

    void expanded_flaw() override;

  protected:
    [[nodiscard]] json::json to_json() const override;

  private:
    const utils::lit phi; // the literal indicating whether the flaw is active or not..
    const utils::var pos; // the position variable associated to this flaw (for avoiding causality loops)..
  };

  class stresolver : public resolver
  {
  public:
    stresolver(flaw &f, utils::rational &&intrinsic_cost) noexcept;
    stresolver(flaw &f, utils::rational &&intrinsic_cost, const utils::lit &rho) noexcept;

    [[nodiscard]] const utils::lit &get_rho() const noexcept { return rho; }

  protected:
    void add(const utils::lit &e);

    [[nodiscard]] json::json to_json() const override;

  private:
    const utils::lit rho; // the literal indicating whether the resolver is active or not..
  };
} // namespace ratio
