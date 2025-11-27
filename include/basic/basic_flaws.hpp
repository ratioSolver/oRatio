#pragma once

#include "basic_solver.hpp"

namespace ratio
{
  class solver;
  class resolver;

  class flaw
  {
    friend class solver;
    friend class resolver;

  public:
    flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes);
    flaw(const flaw &) = delete;
    virtual ~flaw() = default;

    [[nodiscard]] uintptr_t get_id() const noexcept { return reinterpret_cast<uintptr_t>(this); }

    [[nodiscard]] const std::vector<std::reference_wrapper<resolver>> &get_causes() const noexcept { return causes; }
    [[nodiscard]] const std::vector<std::shared_ptr<resolver>> &get_resolvers() const noexcept { return resolvers; }

    [[nodiscard]] virtual utils::rational get_estimated_cost() const noexcept = 0;

    [[nodiscard]] virtual json::json to_json() const;

    [[nodiscard]] size_t get_position() const noexcept { return position; }

  protected:
    [[nodiscard]] linspire::solver &get_lin() const noexcept { return slv.lin_slv; }
    [[nodiscard]] arc_consistency::solver &get_ac() const noexcept { return slv.ac_slv; }

    template <typename Tp, typename... Args>
    Tp &new_resolver(Args &&...args) noexcept
    {
      static_assert(std::is_base_of_v<resolver, Tp>, "Tp must be a subclass of resolver");
      auto r = std::make_unique<Tp>(std::forward<Args>(args)...);
      auto &r_ref = *r;
      resolvers.emplace_back(std::move(r));
      return r_ref;
    }

  private:
    virtual void compute_resolvers() = 0;

  protected:
    solver &slv; // the solver this flaw belongs to..

  private:
    std::vector<std::reference_wrapper<resolver>> causes; // the causes of this flaw..
    std::vector<std::shared_ptr<resolver>> resolvers;     // the resolvers for this flaw..
    size_t position = 0;                                  // the position of the flaw in the solver..
  };

  class resolver
  {
    friend class solver;
    friend class flaw;

  public:
    resolver(flaw &flw, utils::rational &&intrinsic_cost);
    resolver(const resolver &) = delete;
    virtual ~resolver() = default;

    [[nodiscard]] uintptr_t get_id() const noexcept { return reinterpret_cast<uintptr_t>(this); }

    [[nodiscard]] flaw &get_flaw() const noexcept { return flw; }

    [[nodiscard]] const utils::rational &get_intrinsic_cost() const noexcept { return intrinsic_cost; }

    [[nodiscard]] const std::vector<std::reference_wrapper<flaw>> &get_preconditions() const noexcept { return preconditions; }

    [[nodiscard]] utils::rational get_estimated_cost() const noexcept;

    [[nodiscard]] virtual json::json to_json() const;

  protected:
    [[nodiscard]] solver &get_solver() noexcept { return flw.slv; }
    [[nodiscard]] bool execute(const riddle::bool_expr &expr) noexcept { return get_solver().execute(expr, ctx); }

    [[nodiscard]] linspire::solver &get_lin() noexcept { return flw.slv.lin_slv; }
    [[nodiscard]] arc_consistency::solver &get_ac() noexcept { return flw.slv.ac_slv; }
    void add_constraint(arc_consistency::constraint &cnstr) noexcept
    {
      ctx.ac_cnsts.push_back(cnstr);
      get_ac().add_constraint(cnstr);
    }

  private:
    [[nodiscard]] virtual bool apply() noexcept = 0;

  protected:
    flaw &flw;   // the flaw solved by this resolver..
    context ctx; // the context in which this resolver is applied..

  private:
    utils::rational intrinsic_cost;                          // the intrinsic cost of this resolver..
    std::vector<std::reference_wrapper<flaw>> preconditions; // the preconditions of this resolver..
  };

  class enum_flaw final : public flaw
  {
    friend class enum_item;

  public:
    enum_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev) noexcept;

    utils::rational get_estimated_cost() const noexcept override;

    [[nodiscard]] const riddle::enum_expr &get_var() const noexcept { return var; }

  private:
    void compute_resolvers() override;

  private:
    bool expanded = false; // whether the resolvers have been computed..
    riddle::enum_expr var;
  };

  class choose_val final : public resolver
  {
    friend class enum_item;

  public:
    choose_val(enum_flaw &f, riddle::expr val) noexcept;

    [[nodiscard]] riddle::expr get_value() const noexcept { return val; }

  private:
    bool apply() noexcept override;

  private:
    riddle::expr val;
  };

  class clause_flaw final : public flaw
  {
  public:
    clause_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause) noexcept;

    utils::rational get_estimated_cost() const noexcept override;

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
    disjunction_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept;

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
    atom_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept;

    utils::rational get_estimated_cost() const noexcept override;

    [[nodiscard]] const riddle::atom_expr &get_atom() const noexcept { return atm; }

  private:
    void compute_resolvers() override;

    [[nodiscard]] static bool have_common_ancestors(const riddle::atom_expr &ancestor, const riddle::atom_expr &descendant);

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
