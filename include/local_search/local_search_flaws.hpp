#pragma once

#include "solver.hpp"

namespace ratio
{
  class ls_flaw : public riddle::flaw
  {
  public:
    ls_flaw(solver &cr, std::vector<std::shared_ptr<resolver>> &&causes, const utils::lit &phi);

    const utils::lit &get_phi() const noexcept { return phi; }

  private:
    const utils::lit phi;
  };

  class ls_resolver : public resolver
  {
  public:
    ls_resolver(ls_flaw &flw, utils::rational &&intrinsic_cost, const utils::lit &rho);

    const utils::lit &get_rho() const noexcept { return rho; }

  private:
    const utils::lit rho;
  };

  class enum_flaw final : public ls_flaw
  {
  public:
    enum_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, const utils::lit &phi, riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev) noexcept;

    utils::rational get_estimated_cost() const noexcept override;

    [[nodiscard]] const riddle::enum_expr &get_var() const noexcept { return var; }

  private:
    void compute_resolvers() override;

  private:
    riddle::enum_expr var;
  };

  class clause_flaw final : public ls_flaw
  {
  public:
    clause_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, const utils::lit &phi, std::vector<riddle::bool_expr> &&clause) noexcept;

    utils::rational get_estimated_cost() const noexcept override;

    [[nodiscard]] const std::vector<riddle::bool_expr> &get_clause() const noexcept { return clause; }

  private:
    void compute_resolvers() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    std::vector<riddle::bool_expr> clause;
  };

  class disjunction_flaw final : public ls_flaw
  {
  public:
    disjunction_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, const utils::lit &phi, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept;

    utils::rational get_estimated_cost() const noexcept override;

    [[nodiscard]] const std::vector<std::unique_ptr<riddle::conjunction>> &get_disjuncts() const noexcept { return disjuncts; }

  private:
    void compute_resolvers() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    std::vector<std::unique_ptr<riddle::conjunction>> disjuncts;
  };
} // namespace ratio
