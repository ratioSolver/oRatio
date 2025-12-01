#pragma once

#include "basic_solver.hpp"
#include "flaw.hpp"

namespace ratio
{
  class flaw : public riddle::flaw
  {
    friend class solver;
    friend class resolver;

  public:
    flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes);

  private:
    virtual void compute_resolvers() = 0;
  };

  class resolver : public riddle::resolver
  {
    friend class solver;
    friend class flaw;

  public:
    resolver(flaw &flw, utils::rational &&intrinsic_cost);

  private:
    [[nodiscard]] virtual bool apply() noexcept = 0;

  protected:
    linspire::constraint lin_cnsts;                                            // The linear constraints in the current context..
    std::vector<std::reference_wrapper<arc_consistency::constraint>> ac_cnsts; // The arc consistency constraints in the current context..
  };

  class enum_flaw final : public flaw
  {

  public:
    enum_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev) noexcept;

    utils::rational get_estimated_cost() const noexcept override;

    [[nodiscard]] const riddle::enum_expr &get_var() const noexcept { return var; }

  private:
    void compute_resolvers() override;

  private:
    riddle::enum_expr var;
  };
} // namespace ratio
