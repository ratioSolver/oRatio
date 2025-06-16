#pragma once

#include "stsolver.hpp"
#include "dl_theory.hpp"

namespace ratio
{
  class stflaw : public flaw, private smt::prop_listener, private smt::dl_listener
  {
  public:
    stflaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, const bool &exclusive = false) noexcept;
    stflaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, const utils::lit &phi, const utils::var &pos, const bool &exclusive = false) noexcept;

    [[nodiscard]] inline solver &get_solver() noexcept { return static_cast<solver &>(get_graph()); }
    [[nodiscard]] inline const solver &get_solver() const noexcept { return static_cast<const solver &>(get_graph()); }

    [[nodiscard]] const utils::lit &get_phi() const noexcept { return phi; }

    [[nodiscard]] const utils::var &get_pos() const noexcept { return pos; }

  private:
    [[nodiscard]] static utils::lit compute_phi(solver &slv, const std::vector<std::reference_wrapper<resolver>> &causes) noexcept;

    void expanded_flaw() override;

    void on_change(const utils::var &v) noexcept override;
    void on_reset(const utils::var &v) noexcept override;
    void on_tp_change(const utils::var &v) noexcept override;

  protected:
    [[nodiscard]] json::json to_json() const override;

  private:
    const utils::lit phi; // the literal indicating whether the flaw is active or not..
    const utils::var pos; // the position variable associated to this flaw (for avoiding causality loops)..
  };

  class stresolver : public resolver, private smt::prop_listener
  {
  public:
    stresolver(stflaw &f, utils::rational &&intrinsic_cost) noexcept;
    stresolver(stflaw &f, utils::rational &&intrinsic_cost, const utils::lit &rho) noexcept;

    [[nodiscard]] inline solver &get_solver() noexcept { return static_cast<solver &>(get_flaw().get_graph()); }
    [[nodiscard]] inline const solver &get_solver() const noexcept { return static_cast<const solver &>(get_flaw().get_graph()); }

    [[nodiscard]] const utils::lit &get_rho() const noexcept { return rho; }

  protected:
    [[nodiscard]] json::json to_json() const override;

  private:
    void on_change(const utils::var &v) noexcept override;
    void on_reset(const utils::var &v) noexcept override;

  private:
    const utils::lit rho; // the literal indicating whether the resolver is active or not..
  };

  class clause_flaw final : public stflaw
  {
  public:
    clause_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<utils::lit> &&clause, const bool &exclusive) noexcept;

    [[nodiscard]] const std::vector<utils::lit> &get_disjuncts() const noexcept { return clause; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<utils::lit> clause;
  };

  class choose_lit final : public stresolver
  {
  public:
    choose_lit(clause_flaw &f, const utils::lit &conj) noexcept;

  private:
    void apply() override;
  };

  class enum_flaw final : public stflaw
  {
  public:
    enum_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::component_type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values) noexcept;

    [[nodiscard]] const std::shared_ptr<riddle::enum_item> &get_var() const noexcept { return var; }

  private:
    void compute_resolvers() override;

    static std::shared_ptr<riddle::enum_item> create_var(riddle::component_type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values);

  private:
    std::shared_ptr<riddle::enum_item> var;
  };

  class choose_val final : public stresolver
  {
  public:
    choose_val(enum_flaw &f, const utils::enum_val &val) noexcept;

  private:
    void apply() override;

  private:
    const utils::enum_val &val;
  };

  class disjunction_flaw final : public stflaw
  {
  public:
    disjunction_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts) noexcept;

    [[nodiscard]] const std::vector<utils::u_ptr<riddle::conjunction>> &get_disjuncts() const noexcept { return disjuncts; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<utils::u_ptr<riddle::conjunction>> disjuncts;
  };

  class choose_conjunction final : public stresolver
  {
  public:
    choose_conjunction(disjunction_flaw &f, riddle::conjunction &conj) noexcept;

  private:
    void apply() override;

  private:
    riddle::conjunction &conj;
  };

  class atom_flaw final : public stflaw
  {
  public:
    atom_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args) noexcept;

    [[nodiscard]] atom_expr get_atom() const noexcept { return atm; }

  private:
    void compute_resolvers() override;

    json::json to_json() const override;

  private:
    atom_expr atm; // the atom that is the subject of the flaw..
  };

  class activate_fact final : public stresolver
  {
  public:
    activate_fact(atom_flaw &f) noexcept;
    activate_fact(atom_flaw &f, const utils::lit &rho) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;
  };

  class activate_goal final : public stresolver
  {
  public:
    activate_goal(atom_flaw &f) noexcept;
    activate_goal(atom_flaw &f, const utils::lit &rho) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;
  };

  class unify_atom final : public stresolver
  {
  public:
    unify_atom(atom_flaw &f, atom_expr atm) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;

  private:
    atom_expr atm; // the atom to unify with..
  };

  class mutex_flaw final : public stflaw
  {
  public:
    /**
     * @brief Constructs a mutex flaw.
     *
     * This flaw appends the `f` flaw to the `r` resolver's preconditions.
     * The resolvers for this flaw are the `f` flaw's resolvers, except the ones that are mutex with the `r` resolver.
     *
     * @param r The resolver that is mutex with at least one resolver of the `f` flaw.
     * @param f The flaw whose at least one resolver is mutex with the `r` resolver.
     */
    mutex_flaw(resolver &r, flaw &f) noexcept;

  private:
    void compute_resolvers() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    resolver &r; // the resolver that is mutex with at least one resolver of the `f` flaw..
    flaw &f;     // the flaw whose at least one resolver is mutex with the `r` resolver..
  };

  class mutex_resolver final : public stresolver
  {
  public:
    mutex_resolver(mutex_flaw &f, const stresolver &r) noexcept;

  private:
    void apply() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    const stresolver &r; // the resolver..
  };
} // namespace ratio
