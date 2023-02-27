#pragma once

#include "oratiosolver_export.h"
#include "core.h"
#include "item.h"
#include "sat_stack.h"
#include "lra_theory.h"
#include "ov_theory.h"
#include "idl_theory.h"
#include "rdl_theory.h"
#include "graph.h"

#define RATIO_AT "at"
#define RATIO_START "start"
#define RATIO_END "end"
#define RATIO_DURATION "duration"
#define RATIO_IMPULSE "Impulse"
#define RATIO_INTERVAL "Interval"

#ifdef BUILD_LISTENERS
#define FIRE_NEW_FLAW(f) fire_new_flaw(f)
#define FIRE_FLAW_COST_CHANGED(f) fire_flaw_cost_changed(f)
#define FIRE_CURRENT_FLAW(f) fire_current_flaw(f)
#define FIRE_NEW_RESOLVER(r) fire_new_resolver(r)
#define FIRE_CURRENT_RESOLVER(r) fire_current_resolver(r)
#define FIRE_CAUSAL_LINK_ADDED(f, r) fire_causal_link_added(f, r)
#else
#define FIRE_NEW_FLAW(f)
#define FIRE_FLAW_COST_CHANGED(f)
#define FIRE_CURRENT_FLAW(f)
#define FIRE_NEW_RESOLVER(r)
#define FIRE_CURRENT_RESOLVER(r)
#define FIRE_CAUSAL_LINK_ADDED(f, r)
#endif

namespace ratio
{
  class atom_flaw;
  class smart_type;
#ifdef BUILD_LISTENERS
  class solver_listener;
#endif

  /**
   * @brief A class for representing boolean items.
   *
   */
  class bool_item : public riddle::item
  {
  public:
    bool_item(riddle::type &t, const semitone::lit &l) : item(t), l(l) {}

    semitone::lit get_lit() const { return l; }

  private:
    const semitone::lit l;
  };

  /**
   * @brief A class for representing arithmetic items. Arithmetic items can be either integers, reals, or time points.
   *
   */
  class arith_item : public riddle::item
  {
  public:
    arith_item(riddle::type &t, const semitone::lin &l) : item(t), l(l) {}

    semitone::lin get_lin() const { return l; }

  private:
    const semitone::lin l;
  };

  /**
   * @brief A class for representing string items.
   *
   */
  class string_item : public riddle::item, public utils::enum_val
  {
  public:
    string_item(riddle::type &t, const std::string &s = "") : item(t), s(s) {}

    const std::string &get_string() const { return s; }

  private:
    const std::string s;
  };

  /**
   * @brief A class for representing enum items.
   *
   */
  class enum_item : public riddle::item
  {
  public:
    enum_item(riddle::type &t, const semitone::var &ev) : item(t), ev(ev) {}

    semitone::var get_var() const { return ev; }

  private:
    const semitone::var ev;
  };

  /**
   * @brief A class for representing atoms.
   *
   */
  class atom : public riddle::atom_item
  {
    friend class atom_flaw;

  public:
    atom(riddle::predicate &p, bool is_fact, semitone::lit sigma) : atom_item(p, is_fact), sigma(sigma) {}

    semitone::lit get_sigma() const { return sigma; }

  private:
    semitone::lit sigma; // the literal that represents this atom..
    atom_flaw *reason;   // the atom_flaw that caused this atom to be created..
  };

  class solver : public riddle::core, public semitone::theory
  {
    friend class flaw;
    friend class resolver;
    friend class atom_flaw;
    friend class graph;
    friend class smart_type;
#ifdef BUILD_LISTENERS
    friend class solver_listener;
#endif

  public:
    solver(const bool &i = true);
    solver(graph_ptr g, const bool &i = true);

    /**
     * @brief Initialize the solver.
     *
     */
    void init();

    void read(const std::string &script) override;
    void read(const std::vector<std::string> &files) override;

    /**
     * @brief Get the linear-real-arithmetic theory.
     *
     * @return semitone::lra_theory& The linear-real-arithmetic theory.
     */
    semitone::lra_theory &get_lra_theory() { return lra_th; }
    /**
     * @brief Get the linear-real-arithmetic theory.
     *
     * @return const semitone::lra_theory& The linear-real-arithmetic theory.
     */
    const semitone::lra_theory &get_lra_theory() const { return lra_th; }
    /**
     * @brief Get the object-variable theory.
     *
     * @return semitone::ov_theory& The object-variable theory.
     */
    semitone::ov_theory &get_ov_theory() { return ov_th; }
    /**
     * @brief Get the object-variable theory.
     *
     * @return const semitone::ov_theory& The object-variable theory.
     */
    const semitone::ov_theory &get_ov_theory() const { return ov_th; }
    /**
     * @brief Get the integer difference logic theory.
     *
     * @return semitone::idl_theory& The integer difference logic theory.
     */
    semitone::idl_theory &get_idl_theory() { return idl_th; }
    /**
     * @brief Get the integer difference logic theory.
     *
     * @return const semitone::idl_theory& The integer difference logic theory.
     */
    const semitone::idl_theory &get_idl_theory() const { return idl_th; }
    /**
     * @brief Get the real difference logic theory.
     *
     * @return semitone::rdl_theory& The real difference logic theory.
     */
    semitone::rdl_theory &get_rdl_theory() { return rdl_th; }
    /**
     * @brief Get the real difference logic theory.
     *
     * @return const semitone::rdl_theory& The real difference logic theory.
     */
    const semitone::rdl_theory &get_rdl_theory() const { return rdl_th; }

    riddle::expr new_bool() override;
    riddle::expr new_bool(bool value) override;

    riddle::expr new_int() override;
    riddle::expr new_int(utils::I value) override;

    riddle::expr new_real() override;
    riddle::expr new_real(utils::rational value) override;

    riddle::expr new_time_point() override;
    riddle::expr new_time_point(utils::rational value) override;

    riddle::expr new_string() override;
    riddle::expr new_string(const std::string &value) override;

    riddle::expr new_item(riddle::complex_type &tp) override;

    riddle::expr new_enum(riddle::type &tp, const std::vector<riddle::expr> &xprs) override;
    riddle::expr get_enum(riddle::expr &xpr, const std::string &name) override;

    riddle::expr add(const std::vector<riddle::expr> &xprs) override;
    riddle::expr sub(const std::vector<riddle::expr> &xprs) override;
    riddle::expr mul(const std::vector<riddle::expr> &xprs) override;
    riddle::expr div(const std::vector<riddle::expr> &xprs) override;
    riddle::expr minus(const riddle::expr &xpr) override;

    riddle::expr lt(const riddle::expr &lhs, const riddle::expr &rhs) override;
    riddle::expr leq(const riddle::expr &lhs, const riddle::expr &rhs) override;
    riddle::expr gt(const riddle::expr &lhs, const riddle::expr &rhs) override;
    riddle::expr geq(const riddle::expr &lhs, const riddle::expr &rhs) override;
    riddle::expr eq(const riddle::expr &lhs, const riddle::expr &rhs) override;
    /**
     * @brief Checks whether the two expressions can be made equal.
     *
     * @param left The first expression to check if it can be made equal to the other.
     * @param right The second expression to check if it can be made equal to the other.
     * @return true If the two expressions can be made equal.
     * @return false If the two expressions can not be made equal.
     */
    bool matches(const riddle::expr &lhs, const riddle::expr &rhs) const noexcept;

    riddle::expr conj(const std::vector<riddle::expr> &xprs) override;
    riddle::expr disj(const std::vector<riddle::expr> &xprs) override;
    riddle::expr exct_one(const std::vector<riddle::expr> &xprs) override;
    riddle::expr negate(const riddle::expr &xpr) override;

    void assert_fact(const riddle::expr &xpr) override;

    void new_disjunction(std::vector<riddle::conjunction_ptr> xprs) override;

    riddle::expr new_fact(riddle::predicate &pred) override;
    riddle::expr new_goal(riddle::predicate &pred) override;

    bool is_constant(const riddle::expr &xpr) const override;

    utils::lbool bool_value(const riddle::expr &xpr) const override;
    utils::inf_rational arith_value(const riddle::expr &xpr) const override;
    std::pair<utils::inf_rational, utils::inf_rational> arith_bounds(const riddle::expr &xpr) const override;
    utils::inf_rational time_value(const riddle::expr &xpr) const override;
    std::pair<utils::inf_rational, utils::inf_rational> time_bounds(const riddle::expr &xpr) const override;

    bool is_enum(const riddle::expr &xpr) const override;
    std::vector<riddle::expr> domain(const riddle::expr &xpr) const override;
    void prune(const riddle::expr &xpr, const riddle::expr &val) override;

    /**
     * @brief Solves the current problem returning whether a solution was found.
     *
     * @return true If a solution was found.
     * @return false If no solution was found.
     */
    bool solve();
    /**
     * @brief Takes a decision and propagates its consequences.
     *
     * @param ch The decision to take.
     */
    void take_decision(const semitone::lit &ch);

    /**
     * @brief Finds the next solution.
     *
     */
    void next();

  private:
    void new_flaw(flaw_ptr f, const bool &enqueue = true); // notifies the solver that a new flaw `f` has been created..
    void new_resolver(resolver_ptr r);                     // notifies the solver that a new resolver `r` has been created..
    void new_causal_link(flaw &f, resolver &r);            // notifies the solver that a new causal link between `f` and `r` has been created..

    void expand_flaw(flaw &f);                    // expands the given flaw, computing its resolvers and applying them..
    void apply_resolver(resolver &r);             // applies the given resolver..
    void set_cost(flaw &f, utils::rational cost); // sets the cost of the given flaw to the given value, storing the old cost in the current layer of the trail..

    void solve_inconsistencies();                                          // checks whether the types have any inconsistency and, in case, solve them..
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_incs(); // collects all the current inconsistencies..

    void reset_smart_types();

    void set_ni(const semitone::lit &v) noexcept
    {
      tmp_ni = ni;
      ni = v;
    }

    void restore_ni() noexcept { ni = tmp_ni; }

    const std::unordered_set<flaw *> &get_active_flaws() const noexcept { return active_flaws; }

    inline const std::vector<std::reference_wrapper<resolver>> get_cause()
    {
      if (res)
        return {*res};
      else
        return {};
    }

  private:
    bool propagate(const semitone::lit &p) override;
    bool check() override;
    void push() override;
    void pop() override;

  public:
    riddle::predicate &get_impulse() const noexcept { return *imp_pred; }
    bool is_impulse(const riddle::type &pred) const noexcept { return pred == *imp_pred; }
    bool is_impulse(const atom &atm) const noexcept { return atm.get_type() == *imp_pred; }
    riddle::predicate &get_interval() const noexcept { return *int_pred; }
    bool is_interval(const riddle::type &pred) const noexcept { return pred == *int_pred; }
    bool is_interval(const atom &atm) const noexcept { return atm.get_type() == *int_pred; }

  private:
    riddle::predicate *imp_pred = nullptr; // the `Impulse` predicate..
    riddle::predicate *int_pred = nullptr; // the `Interval` predicate..
    std::vector<smart_type *> smart_types; // the smart-types..

    semitone::lit tmp_ni;                  // the temporary controlling literal, used for restoring the controlling literal..
    semitone::lit ni = semitone::TRUE_lit; // the current controlling literal..

    semitone::lra_theory lra_th; // the linear-real-arithmetic theory..
    semitone::ov_theory ov_th;   // the object-variable theory..
    semitone::idl_theory idl_th; // the integer difference logic theory..
    semitone::rdl_theory rdl_th; // the real difference logic theory..

    graph_ptr gr;                            // the causal graph..
    resolver *res = nullptr;                 // the current resolver (i.e. the cause for the new flaws)..
    std::unordered_set<flaw *> active_flaws; // the currently active flaws..

    std::unordered_map<semitone::var, std::vector<flaw_ptr>> phis;     // the phi variables (propositional variable to flaws) of the flaws..
    std::unordered_map<semitone::var, std::vector<resolver_ptr>> rhos; // the rho variables (propositional variable to resolver) of the resolvers..

    struct layer
    {
      std::unordered_map<flaw *, utils::rational> old_f_costs; // the old estimated flaws` costs..
      std::unordered_set<flaw *> new_flaws;                    // the just activated flaws..
      std::unordered_set<flaw *> solved_flaws;                 // the just solved flaws..
    };
    std::vector<layer> trail; // the list of taken decisions, with the associated changes made, in chronological order..

#ifdef BUILD_LISTENERS
  private:
    std::vector<solver_listener *> listeners; // the solver listeners..

    void fire_new_flaw(const flaw &f) const;
    void fire_flaw_state_changed(const flaw &f) const;
    void fire_flaw_cost_changed(const flaw &f) const;
    void fire_current_flaw(const flaw &f) const;
    void fire_new_resolver(const resolver &r) const;
    void fire_resolver_state_changed(const resolver &r) const;
    void fire_current_resolver(const resolver &r) const;
    void fire_causal_link_added(const flaw &f, const resolver &r) const;
#endif
  };

  json::json value(const riddle::item &itm) noexcept;
  inline json::json to_json(const utils::rational &rat) noexcept
  {
    json::json j_rat;
    j_rat["num"] = rat.numerator();
    j_rat["den"] = rat.denominator();
    return j_rat;
  }
  inline json::json to_json(const utils::inf_rational &rat) noexcept
  {
    json::json j_rat = to_json(rat.get_rational());
    if (rat.get_infinitesimal() != utils::rational::ZERO)
      j_rat["inf"] = to_json(rat.get_infinitesimal());
    return j_rat;
  }
} // namespace ratio
