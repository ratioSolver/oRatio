#pragma once

#include "solver.hpp"
#include "conjunction.hpp"

namespace ratio
{
  class flaw : public riddle::flaw
  {
  public:
    flaw(solver &cr, std::vector<std::reference_wrapper<riddle::resolver>> &&causes);

    const utils::lit &get_phi() const noexcept { return phi; }

    utils::rational get_estimated_cost() const noexcept override;

  private:
    utils::lit get_phi(const std::vector<std::reference_wrapper<riddle::resolver>> &causes) const noexcept;

  private:
    const utils::lit phi;
    utils::rational est_cost = utils::rational::positive_infinite; // the estimated cost of this flaw..
  };

  class resolver : public virtual riddle::resolver
  {
    friend class solver;

  public:
    resolver(flaw &flw, utils::rational &&intrinsic_cost);
    resolver(flaw &flw, utils::rational &&intrinsic_cost, const utils::lit &rho);

    const utils::lit &get_rho() const noexcept { return rho; }

  protected:
    linspire::constraint lin_cnsts;                                            // The linear constraints in the current context..
    std::vector<std::reference_wrapper<arc_consistency::constraint>> ac_cnsts; // The arc consistency constraints in the current context..

  private:
    const utils::lit rho;
  };

  class enum_flaw final : public flaw
  {
  public:
    enum_flaw(solver &slv, std::vector<std::reference_wrapper<riddle::resolver>> &&causes, riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev) noexcept;

    [[nodiscard]] const riddle::enum_expr &get_var() const noexcept { return var; }

  private:
    void compute_resolvers() override;

  private:
    riddle::enum_expr var;
  };

  class select_value final : public resolver, public riddle::select_value
  {
    friend class enum_item;

  public:
    select_value(enum_flaw &flw, const utils::lit &rho, riddle::expr val) noexcept;

  private:
    bool apply() noexcept override;
  };

  class clause_flaw final : public flaw
  {
  public:
    clause_flaw(solver &slv, std::vector<std::reference_wrapper<riddle::resolver>> &&causes, std::vector<riddle::bool_expr> &&clause) noexcept;

    [[nodiscard]] const std::vector<riddle::bool_expr> &get_clause() const noexcept { return clause; }

  private:
    void compute_resolvers() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    std::vector<riddle::bool_expr> clause;
  };

  class choose_lit final : public resolver
  {
  public:
    choose_lit(clause_flaw &flw, riddle::bool_expr lit) noexcept;

  private:
    bool apply() noexcept override;

    [[nodiscard]] json::json to_json() const override;

  private:
    riddle::bool_expr lit; // the literal to choose..
  };

  class disjunction_flaw final : public flaw
  {
  public:
    disjunction_flaw(solver &slv, std::vector<std::reference_wrapper<riddle::resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept;

    [[nodiscard]] const std::vector<std::unique_ptr<riddle::conjunction>> &get_disjuncts() const noexcept { return disjuncts; }

  private:
    void compute_resolvers() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    std::vector<std::unique_ptr<riddle::conjunction>> disjuncts;
  };

  class choose_conjunction final : public resolver
  {
  public:
    choose_conjunction(disjunction_flaw &flw, riddle::conjunction &conj) noexcept;

  private:
    bool apply() noexcept override;

    [[nodiscard]] json::json to_json() const override;

  private:
    riddle::conjunction &conj;
  };

  class atom_flaw final : public flaw
  {
  public:
    atom_flaw(solver &slv, std::vector<std::reference_wrapper<riddle::resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, riddle::bool_expr &&sigma) noexcept;

    [[nodiscard]] const riddle::atom_expr &get_atom() const noexcept { return atm; }

  private:
    void compute_resolvers() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    riddle::atom_expr atm;
  };

  class activate_fact final : public resolver
  {
  public:
    activate_fact(atom_flaw &flw) noexcept;
    activate_fact(atom_flaw &flw, const utils::lit &rho) noexcept;

  private:
    bool apply() noexcept override;

    json::json to_json() const override;
  };

  class activate_goal final : public resolver
  {
  public:
    activate_goal(atom_flaw &flw) noexcept;
    activate_goal(atom_flaw &flw, const utils::lit &rho) noexcept;

  private:
    bool apply() noexcept override;

    json::json to_json() const override;
  };

  class unify_atom final : public resolver
  {
  public:
    unify_atom(atom_flaw &flw, riddle::atom_expr atm) noexcept;

  private:
    bool apply() noexcept override;

    json::json to_json() const override;

  private:
    riddle::atom_expr atm; // the atom to unify with..
  };

  class sv_peak final : public flaw
  {
  public:
    sv_peak(solver &slv, std::vector<riddle::atom_expr> &&atms) noexcept;

  private:
    void compute_resolvers() noexcept override;

  private:
    std::vector<riddle::atom_expr> atms;
  };

  class rr_peak final : public flaw
  {
  public:
    rr_peak(solver &slv, std::vector<riddle::atom_expr> &&atms) noexcept;

  private:
    void compute_resolvers() noexcept override;

  private:
    std::vector<riddle::atom_expr> atms;
  };

  class cr_overproduction final : public flaw
  {
  public:
    cr_overproduction(solver &slv, std::vector<riddle::atom_expr> &&prod_atms, std::vector<riddle::atom_expr> &&cons_atms) noexcept;

  private:
    void compute_resolvers() noexcept override;

  private:
    std::vector<riddle::atom_expr> prod_atms;
    std::vector<riddle::atom_expr> cons_atms;
  };

  class cr_overconsumption final : public flaw
  {
  public:
    cr_overconsumption(solver &slv, std::vector<riddle::atom_expr> &&cons_atms, std::vector<riddle::atom_expr> &&prod_atms) noexcept;

  private:
    void compute_resolvers() noexcept override;

  private:
    std::vector<riddle::atom_expr> cons_atms;
    std::vector<riddle::atom_expr> prod_atms;
  };

  class ordering final : public resolver
  {
  public:
    ordering(flaw &flw, riddle::atom_expr before, riddle::atom_expr after) noexcept;

  private:
    bool apply() noexcept override;

  private:
    riddle::atom_expr before;
    riddle::atom_expr after;
  };
} // namespace ratio
