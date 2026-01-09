#pragma once

#include "graph.hpp"
#include "linspire.hpp"
#include "arc_consistency.hpp"
#include "types.hpp"

namespace ratio
{
  static constexpr const char *INIT_STRING = "predicate Impulse(real at) { at >= origin; at <= horizon; } predicate Interval(real start, real end, real duration) { start >= origin; duration == end - start; duration >= 0.0; end <= horizon; } real origin, horizon; origin >= 0.0; origin <= horizon;";

  class flaw;
  class resolver;
  class activate_fact;
  class activate_goal;
  class unify_atom;

  class solver : public riddle::graph
  {
    friend class flaw;
    friend class resolver;
    friend class activate_fact;
    friend class activate_goal;
    friend class unify_atom;

  public:
    solver(std::string_view name = "oRatio") noexcept;

    void read(std::string_view script) override;
    void read(const std::vector<std::filesystem::path> &files) override;

    [[nodiscard]] riddle::bool_expr new_bool() override;
    [[nodiscard]] riddle::bool_expr new_bool(const bool value) override;
    [[nodiscard]] utils::lbool bool_value(const riddle::bool_term &expr) const noexcept override;

    [[nodiscard]] riddle::arith_expr new_int() override;
    [[nodiscard]] riddle::arith_expr new_int(const INT_TYPE value) override;
    [[nodiscard]] riddle::arith_expr new_int(const INT_TYPE lb, const INT_TYPE ub) override;
    [[nodiscard]] riddle::arith_expr new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub) override;

    [[nodiscard]] riddle::arith_expr new_real() override;
    [[nodiscard]] riddle::arith_expr new_real(utils::rational &&value) override;
    [[nodiscard]] riddle::arith_expr new_real(utils::rational &&lb, utils::rational &&ub) override;
    [[nodiscard]] riddle::arith_expr new_uncertain_real(utils::rational &&lb, utils::rational &&ub) override;

    [[nodiscard]] riddle::arith_expr new_time() override;
    [[nodiscard]] riddle::arith_expr new_time(utils::rational &&value) override;

    [[nodiscard]] utils::inf_rational arith_value(const riddle::arith_term &xpr) const noexcept override;
    bool is_constant(const riddle::arith_term &xpr) const noexcept override;

    [[nodiscard]] riddle::string_expr new_string() override;
    [[nodiscard]] riddle::string_expr new_string(std::string &&value) override;
    [[nodiscard]] std::string string_value(const riddle::string_term &xpr) const noexcept override;

    [[nodiscard]] std::unordered_set<riddle::expr> enum_value(const riddle::enum_term &xpr) const noexcept override;

    [[nodiscard]] riddle::arith_expr new_negation(riddle::arith_expr xpr) override;

    [[nodiscard]] riddle::arith_expr new_sum(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_subtraction(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_product(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_division(std::vector<riddle::arith_expr> &&xprs) override;

    [[nodiscard]] riddle::expr new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values) override;

    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) override;
    void new_clause(std::vector<riddle::bool_expr> &&exprs) override;

    [[nodiscard]] bool match(riddle::term &lhs, riddle::term &rhs) const;

    void solve();

  protected:
    [[nodiscard]] utils::var new_prop() noexcept override;
    [[nodiscard]] utils::lbool prop_val(const utils::lit &l) const noexcept override;
    void new_clause(std::vector<utils::lit> &&lits) noexcept override;

  private:
    void new_enum_eq(const riddle::enum_term &xpr, const utils::enum_val &val, riddle::expr lhs, riddle::expr rhs) noexcept override;
    [[nodiscard]] riddle::atom_expr create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::term>, std::less<>> &&args) override;

  private:
    bool mk_assign(riddle::bool_expr xpr, utils::lbool val) noexcept override;
    bool mk_eq(riddle::bool_expr lhs, riddle::bool_expr rhs) noexcept override;
    bool mk_neq(riddle::bool_expr lhs, riddle::bool_expr rhs) noexcept override;

    bool mk_lt(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept override;
    bool mk_le(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept override;
    bool mk_eq(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept override;
    bool mk_neq(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept override;
    bool mk_ge(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept override;
    bool mk_gt(riddle::arith_expr lhs, riddle::arith_expr rhs) noexcept override;

    bool mk_assign(riddle::enum_expr xpr, const utils::enum_val &val) noexcept override;
    bool mk_forbid(riddle::enum_expr xpr, const utils::enum_val &val) noexcept override;
    bool mk_eq(riddle::enum_expr lhs, riddle::enum_expr rhs) noexcept override;
    bool mk_neq(riddle::enum_expr lhs, riddle::enum_expr rhs) noexcept override;

#ifdef RIDDLE_ENABLE_LISTENERS
  private:
    /**
     * @brief This function is called when the state changes.
     *
     * This function should be overridden by derived classes to handle the state change event.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void state_changed() {}

    /**
     * @brief Notifies when the state of a flaw has changed.
     *
     * This function is called when the state of a flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw's state changes.
     *
     * @param flaw The flaw whose state has changed.
     */
    virtual void flaw_state_changed(const flaw &) {}

    /**
     * @brief Notifies when the state of a resolver has changed.
     *
     * This function is called when the state of a resolver has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a resolver's state changes.
     *
     * @param resolver The resolver whose state has changed.
     */
    virtual void resolver_state_changed(const resolver &) {}
#endif

  private:
    arc_consistency::solver ac_slv;                // The arc consistency solver..
    linspire::solver lin_slv;                      // The linear programming solver..
    std::unordered_set<riddle::flaw *> open_flaws; // The set of open flaws..
  };

  class state_variable final : public riddle::state_variable
  {
  public:
    state_variable(solver &slv) noexcept;

  private:
    std::shared_ptr<riddle::flaw> new_peak(std::vector<riddle::atom_expr> &&atms) noexcept override;
  };

  class reusable_resource final : public riddle::reusable_resource
  {
  public:
    reusable_resource(solver &slv) noexcept;

  private:
    std::shared_ptr<riddle::flaw> new_peak(std::vector<riddle::atom_expr> &&atms) noexcept override;
  };

  class consumable_resource final : public riddle::consumable_resource
  {
  public:
    consumable_resource(solver &slv) noexcept;

  private:
    std::shared_ptr<riddle::flaw> new_overproduction(std::vector<riddle::atom_expr> &&prod_atms, std::vector<riddle::atom_expr> &&cons_atms) noexcept override;
    std::shared_ptr<riddle::flaw> new_overconsumption(std::vector<riddle::atom_expr> &&cons_atms, std::vector<riddle::atom_expr> &&prod_atms) noexcept override;
  };
} // namespace ratio
