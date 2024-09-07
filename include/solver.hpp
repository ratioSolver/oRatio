#pragma once

#include <unordered_set>
#include "core.hpp"
#include "sat_core.hpp"
#include "lra_theory.hpp"
#include "idl_theory.hpp"
#include "rdl_theory.hpp"
#include "ov_theory.hpp"

namespace ratio
{
  class atom_flaw;
  class graph;
  class smart_type;
#ifdef BUILD_LISTENERS
  class flaw;
  class resolver;
#endif

  constexpr std::string_view IMPULSE_PREDICATE_NAME = "Impulse";
  constexpr std::string_view INTERVAL_PREDICATE_NAME = "Interval";
  constexpr std::string_view AT_NAME = "at";
  constexpr std::string_view START_NAME = "start";
  constexpr std::string_view END_NAME = "end";
  constexpr std::string_view ORIGIN_NAME = "origin";
  constexpr std::string_view HORIZON_NAME = "horizon";

  class atom : public riddle::atom
  {
    friend class atom_flaw;

  public:
    atom(riddle::predicate &p, bool is_fact, atom_flaw &reason, std::map<std::string, std::shared_ptr<item>, std::less<>> &&args = {});

    [[nodiscard]] atom_flaw &get_reason() const { return reason; }

  private:
    atom_flaw &reason; // the flaw associated to this atom..
  };

  class solver : public riddle::core
  {
    friend class graph;

  public:
    solver(std::string_view name = "oRatio") noexcept;
    virtual ~solver() = default;

    const std::string &get_name() const noexcept { return name; }

    void init();

    void read(const std::string &script) override;
    void read(const std::vector<std::string> &files) override;

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
     * @param d The decision to take.
     */
    void take_decision(const utils::lit &d);

    /**
     * @brief Advances to the next step in the solver.
     *
     * This function is responsible for moving the solver to the next step in the computation.
     * It performs any necessary calculations or updates to the solver's internal state.
     */
    void next();

    [[nodiscard]] semitone::sat_core &get_sat() noexcept { return sat; }
    [[nodiscard]] const semitone::sat_core &get_sat() const noexcept { return sat; }
    [[nodiscard]] semitone::lra_theory &get_lra_theory() noexcept { return lra; }
    [[nodiscard]] const semitone::lra_theory &get_lra_theory() const noexcept { return lra; }
    [[nodiscard]] semitone::idl_theory &get_idl_theory() noexcept { return idl; }
    [[nodiscard]] const semitone::idl_theory &get_idl_theory() const noexcept { return idl; }
    [[nodiscard]] semitone::rdl_theory &get_rdl_theory() noexcept { return rdl; }
    [[nodiscard]] const semitone::rdl_theory &get_rdl_theory() const noexcept { return rdl; }
    [[nodiscard]] semitone::ov_theory &get_ov_theory() noexcept { return ov; }
    [[nodiscard]] const semitone::ov_theory &get_ov_theory() const noexcept { return ov; }
    [[nodiscard]] graph &get_graph() noexcept { return gr; }
    [[nodiscard]] const graph &get_graph() const noexcept { return gr; }

    [[nodiscard]] std::shared_ptr<riddle::bool_item> new_bool() noexcept override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> new_int() noexcept override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> new_real() noexcept override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> new_time() noexcept override;
    [[nodiscard]] std::shared_ptr<riddle::enum_item> new_enum(riddle::type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values) override;

    [[nodiscard]] std::shared_ptr<riddle::arith_item> minus(const std::shared_ptr<riddle::arith_item> &xpr) override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> add(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs) override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> sub(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs) override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> mul(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs) override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> div(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs) override;

    [[nodiscard]] std::shared_ptr<riddle::bool_item> lt(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> leq(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> gt(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> geq(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> eq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;

    [[nodiscard]] bool matches(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) const noexcept override;

    [[nodiscard]] std::shared_ptr<riddle::bool_item> conj(const std::vector<std::shared_ptr<riddle::bool_item>> &exprs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> disj(const std::vector<std::shared_ptr<riddle::bool_item>> &exprs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> exct_one(const std::vector<std::shared_ptr<riddle::bool_item>> &exprs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> negate(const std::shared_ptr<riddle::bool_item> &expr) override;

    void assert_fact(const std::shared_ptr<riddle::bool_item> &fact) override;

    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept override;

    [[nodiscard]] std::shared_ptr<riddle::atom> new_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>, std::less<>> &&arguments = {}) noexcept override;

    [[nodiscard]] utils::lbool bool_value(const riddle::bool_item &expr) const noexcept override;
    [[nodiscard]] utils::inf_rational arithmetic_value(const riddle::arith_item &expr) const noexcept override;
    [[nodiscard]] std::pair<utils::inf_rational, utils::inf_rational> bounds(const riddle::arith_item &expr) const noexcept override;
    [[nodiscard]] std::vector<std::reference_wrapper<utils::enum_val>> domain(const riddle::enum_item &expr) const noexcept override;
    [[nodiscard]] bool assign(const riddle::enum_item &expr, utils::enum_val &val) override;
    void forbid(const riddle::enum_item &expr, utils::enum_val &val) override;

  private:
    /**
     * @brief Solves inconsistencies in the system.
     *
     * This function is responsible for resolving any inconsistencies that may exist in the system.
     */
    void solve_inconsistencies();

    /**
     * @brief Get the current inconsistencies of the solver.
     *
     * This function returns the current inconsistencies of the solver.
     * The inconsistencies are represented as a vector of vectors of pairs of literals and doubles.
     * Each vector of pairs represents a possible decision which can be taken to resolve the inconsistency.
     * The pairs represent the literals and the costs of the literals.
     *
     * @return std::vector<std::vector<std::pair<utils::lit, double>>> The current inconsistencies of the solver.
     */
    [[nodiscard]] std::vector<std::vector<std::pair<utils::lit, double>>> get_incs() const noexcept;

    /**
     * @brief Resets the smart types.
     *
     * This function resets the smart types used by the solver.
     */
    void reset_smart_types() noexcept;

#ifdef BUILD_LISTENERS
    /**
     * @brief This function is called when the state of the solver changes.
     *
     * This function should be overridden by derived classes to handle the state change event.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void state_changed() {}
    /**
     * @brief Notifies when a flaw has been created.
     *
     * This function is called when a flaw has been created. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw is created.
     *
     * @param flaw The flaw that has been created.
     */
    virtual void flaw_created(const flaw &) {}
    /**
     * @brief Notifies when the state of a flaw has changed.
     *
     * This function is called when the state of a flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw's state changes.
     *
     * @param flaw The flaw whose state has changed.
     */
    virtual void flaw_state_changed(const flaw &) {}
    /**
     * @brief Notifies when the cost of a flaw has changed.
     *
     * This function is called when the cost of a flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw's cost changes.
     *
     * @param flaw The flaw whose cost has changed.
     */
    virtual void flaw_cost_changed(const flaw &) {}
    /**
     * @brief Notifies when the position of a flaw has changed.
     *
     * This function is called when the position of a flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw's position changes.
     *
     * @param flaw The flaw whose position has changed.
     */
    virtual void flaw_position_changed(const flaw &) {}
    /**
     * @brief Notifies when the current flaw has changed.
     *
     * This function is called when the current flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when the current flaw changes.
     *
     * @param flaw The current flaw.
     */
    virtual void current_flaw(const flaw &) {}

    /**
     * @brief Notifies when a resolver has been created.
     *
     * This function is called when a resolver has been created. It is a virtual function that can be overridden by derived classes to perform specific actions when a resolver is created.
     *
     * @param resolver The resolver that has been created.
     */
    virtual void resolver_created(const resolver &) {}
    /**
     * @brief Notifies when the state of a resolver has changed.
     *
     * This function is called when the state of a resolver has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a resolver's state changes.
     *
     * @param resolver The resolver whose state has changed.
     */
    virtual void resolver_state_changed(const resolver &) {}
    /**
     * @brief Notifies when the current resolver has changed.
     *
     * This function is called when the current resolver has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when the current resolver changes.
     *
     * @param resolver The current resolver.
     */
    virtual void current_resolver(const resolver &) {}

    /**
     * @brief Notifies when a causal link has been added.
     *
     * This function is called when a causal link has been added. It is a virtual function that can be overridden by derived classes to perform specific actions when a causal link is added.
     *
     * @param flaw The flaw that is the source of the causal link.
     * @param resolver The resolver that is the destination of the causal link.
     */
    virtual void causal_link_added(const flaw &, const resolver &) {}
#endif

  private:
    [[nodiscard]] std::shared_ptr<riddle::item> get(riddle::enum_item &enm, std::string_view name) noexcept override;

  private:
    const std::string name;                // the name of the solver
    semitone::sat_core sat;                // the SAT core
    semitone::lra_theory &lra;             // the linear real arithmetic theory
    semitone::idl_theory &idl;             // the integer difference logic theory
    semitone::rdl_theory &rdl;             // the real difference logic theory
    semitone::ov_theory &ov;               // the object variable theory
    graph &gr;                             // the causal graph
    std::vector<smart_type *> smart_types; // the smart-types
  };

  /**
   * Checks if the given atom is of type "Impulse".
   *
   * @param atm The atom to check.
   * @return True if the atom is of type "Impulse", false otherwise.
   */
  inline bool is_impulse(const atom &atm) noexcept { return atm.get_core().get_predicate(IMPULSE_PREDICATE_NAME)->get().is_assignable_from(atm.get_type()); }
  /**
   * Checks if the given atom is of type "Interval".
   *
   * @param atm The atom to check.
   * @return True if the atom is of type "Interval", false otherwise.
   */
  inline bool is_interval(const atom &atm) noexcept { return atm.get_core().get_predicate(INTERVAL_PREDICATE_NAME)->get().is_assignable_from(atm.get_type()); }

  /**
   * @brief Gets the unique identifier of the given solver.
   *
   * @param s the solver to get the unique identifier of.
   * @return uintptr_t the unique identifier of the given solver.
   */
  inline uintptr_t get_id(const solver &s) noexcept { return reinterpret_cast<uintptr_t>(&s); }
} // namespace ratio
