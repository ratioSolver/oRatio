#pragma once

#include "core.hpp"
#include "flaw.hpp"
#include "linspire.hpp"
#include "arc_consistency.hpp"

namespace ratio
{
  static constexpr const char *INIT_STRING = "predicate Impulse(real at) { at >= origin; at <= horizon; } predicate Interval(real start, real end, real duration) { start >= origin; duration == end - start; duration >= 0.0; end <= horizon; } real origin, horizon; origin >= 0.0; origin <= horizon;";

  class solver;

  class resolver : virtual public riddle::resolver
  {
    friend class solver;
    friend class flaw;

  public:
    resolver(riddle::flaw &flw, utils::rational &&intrinsic_cost);

    linspire::constraint &get_lin_constraints() noexcept { return lin_cnsts; }
    const std::vector<std::reference_wrapper<arc_consistency::constraint>> &get_ac_constraints() const noexcept { return ac_cnsts; }

  protected:
    linspire::constraint lin_cnsts;                                            // The linear constraints in the current context..
    std::vector<std::reference_wrapper<arc_consistency::constraint>> ac_cnsts; // The arc consistency constraints in the current context..
  };

  class solver : public riddle::core
  {
  public:
    solver(std::string_view name = "oRatio") noexcept;

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

    riddle::atom_state get_atom_state(const riddle::atom_term &atm) const noexcept override;

    virtual void solve() = 0;

    [[nodiscard]] bool match(riddle::term &lhs, riddle::term &rhs) const;

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

  protected:
    arc_consistency::solver ac_slv; // The arc consistency solver..
    linspire::solver lin_slv;       // The linear programming solver..
  };
} // namespace ratio
