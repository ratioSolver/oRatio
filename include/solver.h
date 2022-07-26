#pragma once

#include "oratiosolver_export.h"
#include "core.h"
#include "theory.h"
#include "sat_stack.h"
#include "lra_theory.h"
#include "ov_theory.h"
#include "idl_theory.h"
#include "rdl_theory.h"

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

namespace ratio::solver
{
  class causal_graph;
  class flaw;
  class atom_flaw;
  class resolver;
  class smart_type;
#ifdef BUILD_LISTENERS
  class solver_listener;
#endif

  class solver : public ratio::core::core, public semitone::theory
  {
    friend class causal_graph;
    friend class flaw;
    friend class atom_flaw;
    friend class resolver;
    friend class smart_type;
#ifdef BUILD_LISTENERS
    friend class solver_listener;
#endif

  public:
    ORATIOSOLVER_EXPORT solver(const bool &i = true);
    ORATIOSOLVER_EXPORT solver(std::unique_ptr<causal_graph> gr, const bool &i = true);
    solver(const solver &orig) = delete;
    ORATIOSOLVER_EXPORT ~solver();

    ORATIOSOLVER_EXPORT void read(const std::string &script) override;
    ORATIOSOLVER_EXPORT void read(const std::vector<std::string> &files) override;

    /**
     * Initializes the solver.
     */
    ORATIOSOLVER_EXPORT void init() noexcept;

    ORATIOSOLVER_EXPORT ratio::core::expr new_bool() noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr new_int() noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr new_real() noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr new_time_point() noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr new_string() noexcept override;

    ORATIOSOLVER_EXPORT ratio::core::expr new_enum(ratio::core::type &tp, const std::vector<ratio::core::expr> &allowed_vals) override;
    ORATIOSOLVER_EXPORT ratio::core::expr get_enum(ratio::core::enum_item &var, const std::string &name) override;
    ORATIOSOLVER_EXPORT void remove(ratio::core::expr &var, semitone::var_value &val) override;

    ORATIOSOLVER_EXPORT ratio::core::expr negate(const ratio::core::expr &var) noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr conj(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr disj(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr exct_one(const std::vector<ratio::core::expr> &exprs) noexcept override;

    ORATIOSOLVER_EXPORT ratio::core::expr add(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr sub(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr mult(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr div(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr minus(const ratio::core::expr &ex) noexcept override;

    ORATIOSOLVER_EXPORT ratio::core::expr lt(const ratio::core::expr &left, const ratio::core::expr &right) noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr leq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr eq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr geq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept override;
    ORATIOSOLVER_EXPORT ratio::core::expr gt(const ratio::core::expr &left, const ratio::core::expr &right) noexcept override;
    /**
     * @brief Checks whether the two items can be made equal.
     *
     * @param left The first item to check if it can be made equal to the other.
     * @param right The second expression to check if it can be made equal to the other.
     * @return true If the two items can be made equal.
     * @return false If the two items can not be made equal.
     */
    bool matches(ratio::core::item &left, ratio::core::item &right) noexcept;

    inline semitone::lit get_ni() const noexcept { return ni; }
    inline void set_ni(const semitone::lit &v) noexcept
    {
      tmp_ni = ni;
      ni = v;
    }

    inline void restore_ni() noexcept
    {
      ni = tmp_ni;
    }

    ORATIOSOLVER_EXPORT void new_disjunction(std::vector<std::unique_ptr<ratio::core::conjunction>> conjs) override;

  private:
    void new_atom(ratio::core::expr &atm, const bool &is_fact = true) override;
    void new_flaw(std::unique_ptr<flaw> f, const bool &enqueue = true); // notifies the solver that a new flaw `f` has been created..
    void new_resolver(std::unique_ptr<resolver> r);                     // notifies the solver that a new resolver `r` has been created..
    void new_causal_link(flaw &f, resolver &r);                         // notifies the solver that a new causal link between a flaw `f` and a resolver `r` has been created..

    void expand_flaw(flaw &f);                       // expands the given flaw..
    void apply_resolver(resolver &r);                // applies the given resolver..
    void set_cost(flaw &f, semitone::rational cost); // sets the cost of the given flaw..

    inline const std::vector<resolver *> get_cause()
    {
      if (res)
        return {res};
      else
        return {};
    }

    semitone::lit eq(ratio::core::item &left, ratio::core::item &right) noexcept;

  public:
    ORATIOSOLVER_EXPORT void assert_facts(std::vector<ratio::core::expr> facts) override;
    ORATIOSOLVER_EXPORT void assert_facts(std::vector<semitone::lit> facts);

    /**
     * @brief Get the linear-real-arithmetic theory.
     *
     * @return semitone::lra_theory& The linear-real-arithmetic theory.
     */
    inline semitone::lra_theory &get_lra_theory() noexcept { return lra_th; }
    /**
     * @brief Get the object-variable theory.
     *
     * @return semitone::ov_theory& The object-variable theory.
     */
    inline semitone::ov_theory &get_ov_theory() noexcept { return ov_th; }
    /**
     * @brief Get the integer difference logic theory.
     *
     * @return semitone::idl_theory& The integer difference logic theory.
     */
    inline semitone::idl_theory &get_idl_theory() noexcept { return idl_th; }
    /**
     * @brief Get the real difference logic theory.
     *
     * @return semitone::rdl_theory& The real difference logic theory.
     */
    inline semitone::rdl_theory &get_rdl_theory() noexcept { return rdl_th; }

    ORATIOSOLVER_EXPORT semitone::lbool bool_value([[maybe_unused]] const ratio::core::bool_item &x) const noexcept override;
    ORATIOSOLVER_EXPORT std::pair<semitone::inf_rational, semitone::inf_rational> arith_bounds([[maybe_unused]] const ratio::core::arith_item &x) const noexcept override;
    ORATIOSOLVER_EXPORT semitone::inf_rational arith_value([[maybe_unused]] const ratio::core::arith_item &x) const noexcept override;
    ORATIOSOLVER_EXPORT std::unordered_set<semitone::var_value *> enum_value([[maybe_unused]] const ratio::core::enum_item &x) const noexcept override;

    inline size_t decision_level() const noexcept { return trail.size(); } // returns the current decision level..
    inline bool root_level() const noexcept { return trail.empty(); }      // checks whether the current decision level is root level..

    const causal_graph &get_causal_graph() const noexcept { return *gr; }
    const std::unordered_set<flaw *> &get_active_flaws() const noexcept { return active_flaws; }

    /**
     * Solves the given problem returning whether a solution has been found.
     */
    ORATIOSOLVER_EXPORT bool solve();
    /**
     * Takes the given decision and propagates its effects.
     */
    ORATIOSOLVER_EXPORT void take_decision(const semitone::lit &ch);

  private:
    std::vector<std::unique_ptr<flaw>> flush_pending_flaws();

    void next();

    bool propagate(const semitone::lit &p) override;
    bool check() override;
    void push() override;
    void pop() override;

    void solve_inconsistencies();                                          // checks whether the types have any inconsistency and, in case, solve them..
    std::vector<std::vector<std::pair<semitone::lit, double>>> get_incs(); // collects all the current inconsistencies..

    void reset_smart_types();

  public:
    ORATIOSOLVER_EXPORT ratio::core::predicate &get_impulse() const noexcept;
    ORATIOSOLVER_EXPORT bool is_impulse(const ratio::core::type &pred) const noexcept;
    ORATIOSOLVER_EXPORT bool is_impulse(const ratio::core::atom &atm) const noexcept;
    ORATIOSOLVER_EXPORT ratio::core::predicate &get_interval() const noexcept;
    ORATIOSOLVER_EXPORT bool is_interval(const ratio::core::type &pred) const noexcept;
    ORATIOSOLVER_EXPORT bool is_interval(const ratio::core::atom &atm) const noexcept;

    friend inline semitone::var get_sigma(const solver &s, const ratio::core::atom &atm) { return s.atom_properties.at(&atm).sigma; }
    friend inline atom_flaw &get_reason(const solver &s, const ratio::core::atom &atm) { return *s.atom_properties.at(&atm).reason; }

  private:
    ratio::core::predicate *imp_pred = nullptr; // the `Impulse` predicate..
    ratio::core::predicate *int_pred = nullptr; // the `Interval` predicate..
    std::vector<smart_type *> smart_types;      // the smart-types..

  private:
    semitone::lit tmp_ni;                  // the temporary controlling literal, used for restoring the controlling literal..
    semitone::lit ni = semitone::TRUE_lit; // the current controlling literal..

    semitone::lra_theory lra_th; // the linear-real-arithmetic theory..
    semitone::ov_theory ov_th;   // the object-variable theory..
    semitone::idl_theory idl_th; // the integer difference logic theory..
    semitone::rdl_theory rdl_th; // the real difference logic theory..

    struct atom_prop
    {
      semitone::var sigma; // this variable represents the state of the atom: if the variable is true, the atom is active; if the variable is false, the atom is unified; if the variable is undefined, the atom is not justified..
      atom_flaw *reason;   // the reason for having introduced the atom..
    };
    std::unordered_map<const ratio::core::atom *, atom_prop> atom_properties; // the atoms` properties..

    std::unique_ptr<causal_graph> gr;                 // the causal graph..
    resolver *res = nullptr;                          // the current resolver (i.e. the cause for the new flaws)..
    std::unordered_set<flaw *> active_flaws;          // the currently active flaws..
    std::vector<std::unique_ptr<flaw>> pending_flaws; // pending flaws, waiting for root-level to be initialized..

    struct layer
    {
      std::unordered_map<flaw *, semitone::rational> old_f_costs; // the old estimated flaws` costs..
      std::unordered_set<flaw *> new_flaws;                       // the just activated flaws..
      std::unordered_set<flaw *> solved_flaws;                    // the just solved flaws..
    };
    std::vector<layer> trail; // the list of taken decisions, with the associated changes made, in chronological order..

    std::unordered_map<semitone::var, std::vector<std::unique_ptr<flaw>>> phis;     // the phi variables (propositional variable to flaws) of the flaws..
    std::unordered_map<semitone::var, std::vector<std::unique_ptr<resolver>>> rhos; // the rho variables (propositional variable to resolver) of the resolvers..

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

  ORATIOSOLVER_EXPORT json::json to_json(const solver &rhs) noexcept;
  ORATIOSOLVER_EXPORT json::json to_timelines(solver &rhs) noexcept;

  ORATIOSOLVER_EXPORT json::json to_json(const ratio::core::item &rhs) noexcept;
  ORATIOSOLVER_EXPORT json::json to_json(const std::map<std::string, ratio::core::expr> &vars) noexcept;
  ORATIOSOLVER_EXPORT json::json value(const ratio::core::item &rhs) noexcept;
} // namespace ratio::solver
