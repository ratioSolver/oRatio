#pragma once

#include "items.hpp"
#include "conjunction.hpp"
#include "linspire.hpp"
#include "arc_consistency.hpp"
#include <vector>
#include <functional>

namespace ratio
{
  class solver;
  class resolver;

  class flaw : private arc_consistency::listener
  {
    friend class solver;
    friend class resolver;

  public:
    flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, const bool &exclusive = false) noexcept;
    flaw(const flaw &) = delete;
    virtual ~flaw() = default;

    [[nodiscard]] uintptr_t get_id() const noexcept { return reinterpret_cast<uintptr_t>(this); }

    [[nodiscard]] const utils::lit &get_phi() const noexcept { return phi; }

    [[nodiscard]] utils::lbool get_state() const noexcept;

    [[nodiscard]] const std::vector<std::reference_wrapper<resolver>> &get_causes() const noexcept { return causes; }

    [[nodiscard]] bool is_expanded() const noexcept { return expanded; }

    [[nodiscard]] const std::vector<std::reference_wrapper<resolver>> &get_resolvers() const noexcept { return resolvers; }

    [[nodiscard]] const utils::rational &get_estimated_cost() const noexcept { return est_cost; }

    [[nodiscard]] const std::vector<std::reference_wrapper<resolver>> &get_supports() const noexcept { return supports; }

    [[nodiscard]] virtual json::json to_json() const;

  protected:
    [[nodiscard]] utils::var new_sat() noexcept;

  private:
    virtual void compute_resolvers() = 0;

    void on_domain_changed(const utils::var v) noexcept override;

  protected:
    solver &slv; // the solver managing this flaw..

  private:
    const utils::lit phi;                                          // the phi literal indicating whether the flaw is active or not..
    std::vector<std::reference_wrapper<resolver>> causes;          // the causes of this flaw..
    bool expanded = false;                                         // whether the flaw has been expanded..
    const bool exclusive;                                          // whether the flaw is exclusive..
    std::vector<std::reference_wrapper<resolver>> resolvers;       // the resolvers for this flaw..
    utils::rational est_cost = utils::rational::positive_infinite; // the current estimated cost of the flaw..
    std::vector<std::reference_wrapper<resolver>> supports;        // the resolvers supported by this flaw (used for propagating cost estimates)..
  };

  class resolver : public arc_consistency::listener
  {
    friend class solver;

  public:
    resolver(flaw &f, utils::rational &&intrinsic_cost) noexcept;
    resolver(flaw &f, utils::rational &&intrinsic_cost, const utils::lit &rho) noexcept;
    resolver(const resolver &) = delete;
    virtual ~resolver() = default;

    [[nodiscard]] uintptr_t get_id() const noexcept { return reinterpret_cast<uintptr_t>(this); }

    [[nodiscard]] flaw &get_flaw() const noexcept { return f; }

    [[nodiscard]] const utils::rational &get_intrinsic_cost() const noexcept { return intrinsic_cost; }

    [[nodiscard]] utils::rational get_estimated_cost() const noexcept;

    [[nodiscard]] const utils::lit &get_rho() const noexcept { return rho; }

    [[nodiscard]] utils::lbool get_state() const noexcept;

    [[nodiscard]] virtual json::json to_json() const;

  private:
    virtual void apply() = 0;

    void on_domain_changed(const utils::var v) noexcept override;

  private:
    flaw &f;                                                            // the flaw solved by this resolver..
    utils::rational intrinsic_cost;                                     // the intrinsic cost of this resolver..
    const utils::lit rho;                                               // the rho literal indicating whether the resolver is active or not..
    std::vector<std::reference_wrapper<flaw>> preconditions;            // the preconditions of this resolver..
    std::shared_ptr<linspire::constraint> cnst;                         // the constraint associated with this resolver..
    std::vector<std::shared_ptr<arc_consistency::constraint>> ac_cnsts; // the arc consistency constraints associated with this resolver..
  };

  class enum_flaw final : public flaw
  {
  public:
    enum_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> var) noexcept;

    [[nodiscard]] const std::shared_ptr<riddle::enum_item> &get_var() const noexcept { return var; }

  private:
    void compute_resolvers() override;

  private:
    std::shared_ptr<riddle::enum_item> var;
  };

  class clause_flaw final : public flaw
  {
  public:
    clause_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause, const bool &exclusive = false) noexcept;

    [[nodiscard]] const std::vector<riddle::bool_expr> &get_clause() const noexcept { return clause; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<riddle::bool_expr> clause;
  };

  class disjunction_flaw final : public flaw
  {
  public:
    disjunction_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept;

    [[nodiscard]] const std::vector<std::unique_ptr<riddle::conjunction>> &get_disjuncts() const noexcept { return disjuncts; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<std::unique_ptr<riddle::conjunction>> disjuncts;
  };

  class atom_flaw final : public flaw
  {
  public:
    atom_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept;

    [[nodiscard]] const riddle::atom_expr &get_atom() const noexcept { return atm; }

    [[nodiscard]] virtual json::json to_json() const override;

  private:
    void compute_resolvers() override;

    bool can_unify_with(const riddle::atom_term &other) const noexcept;

  private:
    riddle::atom_expr atm;
  };

  class activate_fact final : public resolver
  {
  public:
    activate_fact(atom_flaw &f) noexcept;
    activate_fact(atom_flaw &f, const utils::lit &rho) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;
  };

  class activate_goal final : public resolver
  {
  public:
    activate_goal(atom_flaw &f) noexcept;
    activate_goal(atom_flaw &f, const utils::lit &rho) noexcept;

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

  [[nodiscard]] inline std::string to_string(const utils::lbool &node_state) noexcept
  {
    switch (node_state)
    {
    case utils::True:
      return "active";
    case utils::False:
      return "forbidden";
    default:
      return "inactive";
    }
  }
} // namespace ratio
