#pragma once

#include "basic_solver.hpp"
#include "conjunction.hpp"

namespace ratio
{
  class enum_flaw final : public flaw
  {
  public:
    enum_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev) noexcept;

    utils::rational get_estimated_cost() const noexcept override;

    [[nodiscard]] const riddle::enum_expr &get_var() const noexcept { return var; }

  private:
    void compute_resolvers() override;

  private:
    bool expanded = false; // whether the resolvers have been computed..
    riddle::enum_expr var;
  };

  class select_value final : public resolver, public riddle::select_value
  {
    friend class enum_item;

  public:
    select_value(enum_flaw &f, riddle::expr val) noexcept;

  private:
    bool apply() noexcept override;
  };

  class clause_flaw final : public flaw
  {
  public:
    clause_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, std::vector<riddle::const_bool_expr> &&clause) noexcept;

    utils::rational get_estimated_cost() const noexcept override;

    [[nodiscard]] const std::vector<riddle::const_bool_expr> &get_clause() const noexcept { return clause; }

  private:
    void compute_resolvers() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    std::vector<riddle::const_bool_expr> clause;
  };

  class choose_lit final : public resolver
  {
  public:
    choose_lit(clause_flaw &f, riddle::bool_expr lit) noexcept;

  private:
    bool apply() noexcept override;

    [[nodiscard]] json::json to_json() const override;

  private:
    riddle::bool_expr lit; // the literal to choose..
  };

  class disjunction_flaw final : public flaw
  {
  public:
    disjunction_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept;

    utils::rational get_estimated_cost() const noexcept override;

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
    choose_conjunction(disjunction_flaw &f, riddle::conjunction &conj) noexcept;

  private:
    bool apply() noexcept override;

    [[nodiscard]] json::json to_json() const override;

  private:
    riddle::conjunction &conj;
  };

  class atom_flaw final : public flaw
  {
  public:
    atom_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept;

    utils::rational get_estimated_cost() const noexcept override;

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
    activate_fact(atom_flaw &f) noexcept;

  private:
    bool apply() noexcept override;

    json::json to_json() const override;
  };

  class activate_goal final : public resolver
  {
  public:
    activate_goal(atom_flaw &f) noexcept;

  private:
    bool apply() noexcept override;

    json::json to_json() const override;
  };

  class unify_atom final : public resolver
  {
  public:
    unify_atom(atom_flaw &f, riddle::atom_expr atm) noexcept;

  private:
    bool apply() noexcept override;

    json::json to_json() const override;

  private:
    riddle::atom_expr atm; // the atom to unify with..
  };
} // namespace ratio
