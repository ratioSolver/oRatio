#pragma once

#include "solver_core.hpp"

namespace ratio
{
  class basic_solver : public solver_core
  {
  public:
    basic_solver() noexcept;

    [[nodiscard]] riddle::expr new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values) override;

    void new_clause(std::vector<riddle::bool_expr> &&exprs) override;
    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) override;

    void solve() override;

  private:
    [[nodiscard]] riddle::atom_expr create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args) override;

    struct Node
    {
      std::shared_ptr<Node> parent;                        // The parent node..
      std::optional<std::reference_wrapper<resolver>> res; // The resolver applied to reach this node..
      std::unordered_set<flaw *> open_flaws;               // The set of open flaws..
    };
    std::shared_ptr<Node> find_common_ancestor(std::shared_ptr<Node> a, std::shared_ptr<Node> b) const;

    void backtrack_to(const std::shared_ptr<Node> &lca);

    void go_to(const std::shared_ptr<Node> &target);

  private:
    std::shared_ptr<Node> current_node;        // The current node in the search tree..
    std::vector<std::shared_ptr<Node>> fringe; // The fringe of the search tree..
  };

  class enum_flaw final : public flaw
  {
  public:
    enum_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::enum_expr var) noexcept;

    [[nodiscard]] const riddle::enum_expr &get_var() const noexcept { return var; }

  private:
    void compute_resolvers() override;

  private:
    riddle::enum_expr var;
  };

  class clause_flaw final : public flaw
  {
  public:
    clause_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause) noexcept;

    [[nodiscard]] const std::vector<riddle::bool_expr> &get_clause() const noexcept { return clause; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<riddle::bool_expr> clause;
  };

  class choose_lit final : public resolver
  {
  public:
    choose_lit(clause_flaw &f, const utils::lit &conj) noexcept;

  private:
    void apply() override;

  private:
    utils::lit conj; // the literal to choose..
  };

  class disjunction_flaw final : public flaw
  {
  public:
    disjunction_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept;

    [[nodiscard]] const std::vector<std::unique_ptr<riddle::conjunction>> &get_disjuncts() const noexcept { return disjuncts; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<std::unique_ptr<riddle::conjunction>> disjuncts;
  };

  class atom_flaw final : public flaw
  {
  public:
    atom_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept;

    [[nodiscard]] const riddle::atom_expr &get_atom() const noexcept { return atm; }

    [[nodiscard]] json::json to_json() const override;

  private:
    void compute_resolvers() override;

  private:
    riddle::atom_expr atm;
  };

  class activate_fact final : public resolver
  {
  public:
    activate_fact(atom_flaw &f) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;
  };

  class activate_goal final : public resolver
  {
  public:
    activate_goal(atom_flaw &f) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;
  };

  class unify_atom final : public resolver
  {
  public:
    unify_atom(atom_flaw &f, riddle::atom_expr atm) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;

  private:
    riddle::atom_expr atm; // the atom to unify with..
  };
} // namespace ratio
