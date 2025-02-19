#pragma once

#include "stsolver.hpp"
#include "dl_theory.hpp"

namespace ratio
{
  class stflaw : public flaw, private semitone::prop_listener, private semitone::dl_listener
  {
  public:
    stflaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, const bool &exclusive = false) noexcept;

    [[nodiscard]] inline stsolver &get_solver() noexcept { return static_cast<stsolver &>(get_graph()); }
    [[nodiscard]] inline const stsolver &get_solver() const noexcept { return static_cast<const stsolver &>(get_graph()); }

    [[nodiscard]] const utils::lit &get_phi() const noexcept { return phi; }

    [[nodiscard]] const utils::var &get_pos() const noexcept { return pos; }

  private:
    [[nodiscard]] static utils::lit compute_phi(stsolver &slv, const std::vector<utils::ref_wrapper<resolver>> &causes) noexcept;

    void expanded_flaw() override;

    void on_change(const utils::var &v) noexcept override;
    void on_tp_change(const utils::var &v) noexcept override;

  protected:
    [[nodiscard]] json::json to_json() const override;

  private:
    const utils::lit phi; // the literal indicating whether the flaw is active or not..
    const utils::var pos; // the position variable associated to this flaw (for avoiding causality loops)..
  };

  class stresolver : public resolver, private semitone::prop_listener
  {
  public:
    stresolver(flaw &f, utils::rational &&intrinsic_cost) noexcept;
    stresolver(flaw &f, utils::rational &&intrinsic_cost, const utils::lit &rho) noexcept;

    [[nodiscard]] inline stsolver &get_solver() noexcept { return static_cast<stsolver &>(get_flaw().get_graph()); }
    [[nodiscard]] inline const stsolver &get_solver() const noexcept { return static_cast<const stsolver &>(get_flaw().get_graph()); }

    [[nodiscard]] const utils::lit &get_rho() const noexcept { return rho; }

  protected:
    [[nodiscard]] json::json to_json() const override;

  private:
    void on_change(const utils::var &v) noexcept override;

  private:
    const utils::lit rho; // the literal indicating whether the resolver is active or not..
  };

  class stclause_flaw final : public stflaw
  {
  public:
    stclause_flaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, std::vector<utils::lit> &&clause, const bool &exclusive) noexcept;

    [[nodiscard]] const std::vector<utils::lit> &get_disjuncts() const noexcept { return clause; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<utils::lit> clause;
  };

  class stchoose_lit final : public stresolver
  {
  public:
    stchoose_lit(stclause_flaw &f, const utils::lit &conj) noexcept;

  private:
    void apply() override;
  };

  class stenum_flaw final : public stflaw
  {
  public:
    stenum_flaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values) noexcept;

    [[nodiscard]] const utils::s_ptr<riddle::enum_item> &get_var() const noexcept { return var; }

  private:
    void compute_resolvers() override;

    static utils::s_ptr<riddle::enum_item> create_var(riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values);

  private:
    utils::s_ptr<riddle::enum_item> var;
  };

  class stchoose_val final : public stresolver
  {
  public:
    stchoose_val(stenum_flaw &f, const utils::enum_val &val) noexcept;

  private:
    void apply() override;

  private:
    const utils::enum_val &val;
  };

  class stdisjunction_flaw final : public stflaw
  {
  public:
    stdisjunction_flaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts) noexcept;

    [[nodiscard]] const std::vector<utils::u_ptr<riddle::conjunction>> &get_disjuncts() const noexcept { return disjuncts; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<utils::u_ptr<riddle::conjunction>> disjuncts;
  };

  class stchoose_conjunction final : public stresolver
  {
  public:
    stchoose_conjunction(stdisjunction_flaw &f, riddle::conjunction &conj) noexcept;

  private:
    void apply() override;

  private:
    riddle::conjunction &conj;
  };

  class statom_flaw final : public stflaw
  {
  public:
    statom_flaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args) noexcept;

    [[nodiscard]] atom_expr get_atom() const noexcept { return atm; }

  private:
    void compute_resolvers() override;

    json::json to_json() const override;

  private:
    atom_expr atm; // the atom that is the subject of the flaw..
  };

  class stactivate_fact final : public stresolver
  {
  public:
    stactivate_fact(statom_flaw &f) noexcept;
    stactivate_fact(statom_flaw &f, const utils::lit &rho) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;
  };

  class stactivate_goal final : public stresolver
  {
  public:
    stactivate_goal(statom_flaw &f) noexcept;
    stactivate_goal(statom_flaw &f, const utils::lit &rho) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;
  };

  class stunify_atom final : public stresolver
  {
  public:
    stunify_atom(statom_flaw &f, atom_expr atm) noexcept;

  private:
    void apply() override;

    json::json to_json() const override;

  private:
    atom_expr atm; // the atom to unify with..
  };
} // namespace ratio
